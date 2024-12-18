#pragma once

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

#include "hotspot_manager.hpp" // Include the HotspotManager interface

using json = nlohmann::json;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;
namespace websocket = boost::beast::websocket;

class WebSocketServer {
public:
    // Constructor (no message file)
    WebSocketServer(const std::string& ssid, const std::string& password, unsigned short port);
    ~WebSocketServer();

    // Start the server
    void            start();
    // Stop the server
    void            stop();
    // Broadcast arbitrary data to all clients
    bool            broadcastJson(const json& j, const size_t bytes_per_second, const size_t time_ms_between_sends);
    // Enqueue incoming data from clients (thread-safe)
    void            enqueueIncomingData(const std::string& data);
    // Dequeue incoming data (thread-safe), returns empty string if none available
    std::string     dequeueIncomingData();

private:
    // Handle individual client connection
    void            handleClient(std::shared_ptr<websocket::stream<tcp::socket>> ws);
    // Signal handler for graceful shutdown
    static void     signalHandler(int signal, WebSocketServer* server);

    std::unique_ptr<HotspotManager>                                 m_hotspot_manager;
    std::string                                                     m_ssid;
    std::string                                                     m_password;
    unsigned short                                                  m_port;
    asio::io_context                                                m_io_context;
    tcp::acceptor                                                   m_acceptor;
    std::vector<std::shared_ptr<websocket::stream<tcp::socket>>>    m_clients;
    std::mutex                                                      m_clients_mutex;
    std::thread                                                     m_io_thread;
    std::atomic<bool>                                               m_running;
    std::queue<std::string>                                         m_incoming_data_queue;
    std::mutex                                                      m_incoming_data_mutex;
    std::condition_variable                                         m_incoming_data_cv;

    // Prevent copying
    WebSocketServer(const WebSocketServer&) = delete;
    WebSocketServer& operator=(const WebSocketServer&) = delete;
    WebSocketServer(WebSocketServer&&) = delete;
    WebSocketServer& operator=(WebSocketServer&&) = delete;
};
