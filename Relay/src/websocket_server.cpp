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
    start();
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

bool WebSocketServer::broadcastJson(const json& j, const size_t bytes_per_second, const size_t time_ms_between_sends) {

    // Check if JSON contains "messages" array
    if (!j.contains("messages") || !j["messages"].is_array()) {
        std::cerr << "JSON does not contain a 'messages' array.\n";
        return false;
    }

    const auto& messages = j["messages"];
    size_t total_bytes_sent = 0;
    auto start_time = std::chrono::steady_clock::now();
    size_t prev_time_sent_ms = 0;
    size_t hit_data_size_zero_count = 0;

    // going through all messages
    for (unsigned int i = 0; i < messages.size(); /*empty*/ ) {

        //std::cout << i << std::endl;

        // 10 seconds of data size being zero causes to break loop
        if (hit_data_size_zero_count > 10000) {
            std::cout << "data size has been 0 for 10000 iterations. you have to adjust bytes_per_second and time_ms_between_sends\n";
            return false;
        }

        // if total bytes sent exceeds 10GB then the json data is to large and it must be messaged
        if (total_bytes_sent > 10000000) {
            std::cerr << "json data is to large. j.dump().size() is greater than 10GB\n";
            return false;
        }

        if (m_clients.empty()) {
            std::cout << "no clients\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }

        // figure out how long to wait
        size_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();
        long long waiting_ms = (long long)prev_time_sent_ms + (long long)time_ms_between_sends - (long long)now_ms;
        if (waiting_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(waiting_ms)); // waiting until next batch should be sent
            now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();
        }

        // get all messages within current byte limit
        std::string data = "";
        while (total_bytes_sent + data.size() < now_ms*bytes_per_second/1000) {
            if (i == messages.size()) {
                break; // there are no more messages
            }
            data += messages[i].dump();
            ++i;
        }

        // checks if there is data
        if (data.size() == 0) {
            ++hit_data_size_zero_count;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        } else {
            hit_data_size_zero_count = 0;
        }

        // sending data to all clients
        {
            std::lock_guard<std::mutex> lock(m_clients_mutex);
            for (auto it = m_clients.begin(); it != m_clients.end(); ++it ) {
                try {
                    (*it)->write(asio::buffer(data));
                } catch (const std::exception& e) {
                    std::cerr << "Client disconnected during broadcast: " << e.what() << "\n";
                    it = m_clients.erase(it);
                    if (m_clients.empty()) {
                        return false; // No clients left to broadcast to
                    }
                }
            }
        }
        total_bytes_sent += data.size();

        // setting previous time data was sent
        prev_time_sent_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();

    }
    std::cout << "successfully sent all messages. " << total_bytes_sent << " was sent and it took " << prev_time_sent_ms << " milliseconds\n";
    return true;
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
