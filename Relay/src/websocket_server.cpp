#include "websocket_server.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <csignal>

WebSocketServer::WebSocketServer(const std::string& ssid,
                                 const std::string& password,
                                 unsigned short port)
    : m_ssid(ssid),
      m_password(password),
      m_port(port),
      m_acceptor(m_io_context, tcp::endpoint(tcp::v4(), port)),
      m_running(false)
{
    // Instantiate the appropriate HotspotManager based on the platform
    m_hotspot_manager = HotspotManager::createInstance();
}

WebSocketServer::~WebSocketServer() {
    stop();
}

void WebSocketServer::start() {
    if (m_running.load()) {
        std::cerr << "Server is already running.\n";
        return;
    }

    m_running = true;

    // Create hotspot
    try {
        m_hotspot_manager->createHotspot(m_ssid, m_password);
    } catch (const std::exception& e) {
        std::cerr << "Failed to create hotspot: " << e.what() << "\n";
        stop();
        return;
    }

    // Register signal handlers for graceful shutdown
    // Note: Proper signal handling requires access to the server instance.
    // One approach is to store a static instance pointer or use a global variable.
    std::signal(SIGINT, [](int signal) {
        // Placeholder: If you had a global server instance, you'd call server->stop() here.
    });

    // Start accepting clients
    std::thread([this]() {
        try {
            while (m_running.load()) {
                auto socket = std::make_shared<tcp::socket>(m_io_context);
                m_acceptor.accept(*socket);
                auto ws = std::make_shared<websocket::stream<tcp::socket>>(std::move(*socket));
                ws->accept();

                {
                    std::lock_guard<std::mutex> lock(m_clients_mutex);
                    m_clients.push_back(ws);
                }

                // Handle client in a separate thread
                std::thread(&WebSocketServer::handleClient, this, ws).detach();
            }
        } catch (const std::exception& e) {
            if (m_running.load()) {
                std::cerr << "Error accepting connections: " << e.what() << "\n";
            }
        }
    }).detach();

    // Run the io_context in a separate thread
    m_io_thread = std::thread([this]() {
        try {
            m_io_context.run();
        } catch (const std::exception& e) {
            std::cerr << "IO Context error: " << e.what() << "\n";
        }
    });

    std::cout << "WebSocket server started on port " << m_port << ".\n";
}

void WebSocketServer::stop() {
    if (!m_running.load()) {
        return;
    }

    m_running = false;

    // Close all client connections
    {
        std::lock_guard<std::mutex> lock(m_clients_mutex);
        for (auto& ws : m_clients) {
            try {
                ws->close(websocket::close_code::normal);
            } catch (const std::exception& e) {
                std::cerr << "Error closing client connection: " << e.what() << "\n";
            }
        }
        m_clients.clear();
    }

    // Stop io_context
    m_io_context.stop();
    if (m_io_thread.joinable()) {
        m_io_thread.join();
    }

    // Stop hotspot
    try {
        m_hotspot_manager->stopHotspot();
    } catch (const std::exception& e) {
        std::cerr << "Failed to stop hotspot: " << e.what() << "\n";
    }

    std::cout << "WebSocket server stopped.\n";
}

void WebSocketServer::broadcastData(const std::string& data, size_t batch_size) {
    std::lock_guard<std::mutex> lock(m_clients_mutex);
    auto it = m_clients.begin();
    
    while (it != m_clients.end()) {
        size_t count = 0;
        auto end_of_batch = it;
        
        // Send to batch_size clients or until the end of the list
        while (end_of_batch != m_clients.end() && count < batch_size) {
            try {
                (*end_of_batch)->write(asio::buffer(data));
                ++count;
                ++end_of_batch;
            } catch (const std::exception& e) {
                std::cerr << "Client disconnected during batch broadcast: " << e.what() << "\n";
                end_of_batch = m_clients.erase(end_of_batch); // Remove the disconnected client
            }
        }

        // Move the main iterator to where we left off
        it = end_of_batch;

        // Optionally sleep or yield here to prevent blocking
        // std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void WebSocketServer::enqueueIncomingData(const std::string& data) {
    {
        std::lock_guard<std::mutex> lock(m_incoming_data_mutex);
        m_incoming_data_queue.push(data);
    }
    m_incoming_data_cv.notify_one();
}

std::string WebSocketServer::dequeueIncomingData() {
    std::unique_lock<std::mutex> lock(m_incoming_data_mutex);
    if (m_incoming_data_queue.empty()) {
        return "";
    }
    std::string front_data = m_incoming_data_queue.front();
    m_incoming_data_queue.pop();
    return front_data;
}

void WebSocketServer::handleClient(std::shared_ptr<websocket::stream<tcp::socket>> ws) {
    try {
        while (m_running.load()) {
            boost::beast::flat_buffer buffer;
            ws->read(buffer);
            std::string received = boost::beast::buffers_to_string(buffer.data());
            std::cout << "Received from client: " << received << "\n";

            // Enqueue the incoming data for processing elsewhere
            enqueueIncomingData(received);

            // Optionally, echo the received message back
            // ws->write(asio::buffer("Echo: " + received));
        }
    } catch (const std::exception& e) {
        std::cerr << "Client connection error: " << e.what() << "\n";
        std::lock_guard<std::mutex> lock(m_clients_mutex);
        m_clients.erase(std::remove(m_clients.begin(), m_clients.end(), ws), m_clients.end());
    }
}

void WebSocketServer::signalHandler(int signal, WebSocketServer* server) {
    if (signal == SIGINT || signal == SIGTERM) {
        if (server) {
            server->stop();
        }
    }
}
