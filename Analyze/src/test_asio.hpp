#include <boost/asio.hpp>
#include <iostream>
#include <string>

using boost::asio::ip::tcp;

// Function to write data to InfluxDB 2
void write_to_influxdb(const std::string& host,
                       const std::string& port,
                       const std::string& org,
                       const std::string& bucket,
                       const std::string& token,
                       const std::string& data) {
    try {
        boost::asio::io_context io_context;

        // Resolve the host and port
        tcp::resolver resolver(io_context);
        tcp::resolver::results_type endpoints = resolver.resolve(host, port);

        // Create and connect the socket
        tcp::socket socket(io_context);
        boost::asio::connect(socket, endpoints);

        // Construct the HTTP POST request
        std::string request = "POST /api/v2/write?org=" + org +
                              "&bucket=" + bucket + "&precision=ns HTTP/1.1\r\n" +
                              "Host: " + host + ":" + port + "\r\n" +
                              "Authorization: Token " + token + "\r\n" +
                              "Content-Type: text/plain; charset=utf-8\r\n" +
                              "Content-Length: " + std::to_string(data.length()) + "\r\n" +
                              "Connection: close\r\n\r\n" + data;

        // Send the request
        boost::asio::write(socket, boost::asio::buffer(request));

        // Receive the response
        boost::asio::streambuf response;
        boost::system::error_code error;

        // Read until EOF
        boost::asio::read(socket, response, boost::asio::transfer_all(), error);

        if (error != boost::asio::error::eof) {
            throw boost::system::system_error(error);
        }

        // Convert response to string and print
        std::istream response_stream(&response);
        std::string response_str((std::istreambuf_iterator<char>(response_stream)),
                                 std::istreambuf_iterator<char>());
        std::cout << "Response:\n" << response_str << std::endl;
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}