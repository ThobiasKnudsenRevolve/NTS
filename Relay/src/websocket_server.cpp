#include "websocket_server.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <csignal>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <signal.h>
#include <queue>
#include <condition_variable>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

WebSocketServer* WebSocketServer::g_server = nullptr;

WebSocketServer::WebSocketServer() : m_running(false) {
    if (g_server != nullptr) {
        throw std::runtime_error("you cannot create multiple servers.");
    }
    WebSocketServer::g_server = this;
    std::signal(SIGINT, WebSocketServer::signalHandler);
    std::signal(SIGTERM, WebSocketServer::signalHandler);
}

WebSocketServer::~WebSocketServer() {
    stopServer();
    stopHotspot(); // if hotspot is not running this will still work as it should
}

void WebSocketServer::startServer(unsigned short port) {
    if (m_running.load()) {
        std::cerr << "Server is already running.\n";
        return;
    }

    m_port = port;
    m_acceptor = std::make_unique<tcp::acceptor>(m_io_context);
    boost::system::error_code ec;

    m_acceptor->open(tcp::v4(), ec);
    if (ec) {
        std::cerr << "Failed to open acceptor: " << ec.message() << "\n";
        return;
    }

    m_acceptor->set_option(asio::socket_base::reuse_address(true), ec);
    if (ec) {
        std::cerr << "Failed to set reuse_address: " << ec.message() << "\n";
        return;
    }

    m_acceptor->bind(tcp::endpoint(tcp::v4(), m_port), ec);
    if (ec) {
        std::cerr << "Failed to bind: " << ec.message() << "\n";
        return;
    }

    m_acceptor->listen(asio::socket_base::max_listen_connections, ec);
    if (ec) {
        std::cerr << "Failed to listen: " << ec.message() << "\n";
        return;
    }

    m_running = true;

    // Start accepting clients
    std::thread([this]() {
        try {
            while (m_running.load()) {
                auto socket = std::make_shared<tcp::socket>(m_io_context);
                m_acceptor->accept(*socket);
                auto ws = std::make_shared<websocket::stream<tcp::socket>>(std::move(*socket));
                ws->accept();

                {
                    std::lock_guard<std::mutex> lock(m_clients_mutex);
                    m_clients.push_back(ws);
                }

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

void WebSocketServer::stopServer() {
    if (!m_running.load()) {
        return;
    }

    m_running = false;

    stopHotspot();

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

    m_acceptor.reset();

    std::cout << "WebSocket server stopped.\n";
}

void WebSocketServer::startHotspot(const std::string& ssid, const std::string& password) {
    m_ssid = ssid;
    m_password = password;

    // Instantiate hotspot manager here
    if (!m_hotspot_manager) {
        m_hotspot_manager = HotspotManager::createInstance();
    }

    try {
        m_hotspot_manager->createHotspot(m_ssid, m_password);
    } catch (const std::exception& e) {
        std::cerr << "Failed to create hotspot: " << e.what() << "\n";
        return;
    }
}

void WebSocketServer::stopHotspot() {
    try {
        if (m_hotspot_manager) {
            if (m_hotspot_manager->m_hotspot_running) {
                m_hotspot_manager->stopHotspot();
            } else {
                std::cerr << "Hotspot is not active, nothing to stop.\n";
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to stop hotspot: " << e.what() << "\n";
    }
}

bool WebSocketServer::broadcastJson(const json& j, const size_t bytes_per_second, const size_t time_ms_between_sends) {
    if (!m_running.load()) return false;

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

    for (unsigned int i = 0; i < messages.size(); /*empty*/) {

        // if the server is signaled to close this is necessary to escape this function
        if (!m_running.load()) {
            std::cout << "Server stopping, broadcast interrupted.\n";
            return false;
        }

        if (hit_data_size_zero_count > 10000) {
            std::cout << "data size has been 0 for 10000 iterations. you have to adjust bytes_per_second and time_ms_between_sends\n";
            return false;
        }

        if (total_bytes_sent > 10000000) {
            std::cerr << "json data is too large.\n";
            return false;
        }

        if (m_clients.empty()) {
            std::cout << "no clients\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }

        size_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();
        long long waiting_ms = (long long)prev_time_sent_ms + (long long)time_ms_between_sends - (long long)now_ms;
        if (waiting_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(waiting_ms)); 
            now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();
        }

        std::string data = "";
        while (total_bytes_sent + data.size() < now_ms * bytes_per_second / 1000) {
            if (i == messages.size()) {
                break; // no more messages
            }
            data += messages[i].dump();
            ++i;
        }

        if (data.size() == 0) {
            ++hit_data_size_zero_count;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        } else {
            hit_data_size_zero_count = 0;
        }

        std::cout << data << std::endl << std::endl;;

        {
            std::lock_guard<std::mutex> lock(m_clients_mutex);
            for (auto it = m_clients.begin(); it != m_clients.end();) {
                try {
                    (*it)->write(asio::buffer(data));
                    ++it;
                } catch (const std::exception& e) {
                    std::cerr << "Client disconnected during broadcast: " << e.what() << "\n";
                    it = m_clients.erase(it);
                    if (m_clients.empty()) {
                        return false;
                    }
                }
            }
        }
        total_bytes_sent += data.size();

        prev_time_sent_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();
    }
    std::cout << "Successfully sent all messages. " << total_bytes_sent << " bytes sent in " << prev_time_sent_ms << " milliseconds\n";
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

bool WebSocketServer::isRunning() {
    return m_running.load();
}
void WebSocketServer::handleClient(std::shared_ptr<websocket::stream<tcp::socket>> ws) {
    try {
        while (m_running.load()) {
            boost::beast::flat_buffer buffer;
            ws->read(buffer);
            if (!m_running.load()) break;  // Check if still running before processing

            std::string received = boost::beast::buffers_to_string(buffer.data());
            std::cout << "Received from client: " << received << "\n";

            enqueueIncomingData(received);
        }
    } catch (const std::exception& e) {
        std::cerr << "Client connection error: " << e.what() << "\n";
        std::lock_guard<std::mutex> lock(m_clients_mutex);
        m_clients.erase(std::remove(m_clients.begin(), m_clients.end(), ws), m_clients.end());
    }
}

void WebSocketServer::signalHandler(int signal) {
    if (g_server) {
        g_server->stopServer();
        g_server = nullptr;
    }
}