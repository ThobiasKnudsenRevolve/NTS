#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <string>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

int main() {
    try {
        // I/O context
        asio::io_context ioc;
        
        // The server is assumed to run on localhost:PORT
        std::string host = "localhost";
        std::string port = "8080";  // Replace with the actual port number, e.g., "8080"

        // TCP resolver to translate the host/port into endpoints
        tcp::resolver resolver(ioc);
        auto const results = resolver.resolve(host, port);

        // Create a WebSocket stream connected to a TCP socket
        boost::beast::websocket::stream<tcp::socket> ws(ioc);

        // Connect to the resolved endpoint
        asio::connect(ws.next_layer(), results.begin(), results.end());

        // Perform the WebSocket handshake
        ws.handshake(host, "/");

        // Increase the maximum message size (example: 16MB)
        ws.read_message_max(1024ul * 1024ul * 1024ul); // 1 GB

        std::cout << "Connected to server at " << host << ":" << port << "\n";

        // Continuously read messages from the server and print them out
        for (;;) {
            boost::beast::flat_buffer buffer;
            // This call will block until a message is received
            ws.read(buffer);

            // Convert the received buffer to a string and print
            std::string message = boost::beast::buffers_to_string(buffer.data());
            std::cout << "Received: " << message << "\n";
        }

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
    }

    return 0;
}