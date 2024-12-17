
#include "influxdb_client.hpp"
using json = nlohmann::json;

#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <vector>

using boost::asio::ip::tcp;

InfluxdbClient::InfluxdbClient(const std::string& host, const std::string& port, const std::string& org, const std::string& token) : host(host), port(port), org(org), token(token) {}
InfluxdbClient::~InfluxdbClient() {}
bool InfluxdbClient::isConnectedToInfluxdb() const {
    try {
        boost::asio::io_context io_context;
        tcp::resolver resolver(io_context);
        tcp::resolver::results_type endpoints = resolver.resolve(this->host, this->port);
        tcp::socket socket(io_context);
        boost::asio::connect(socket, endpoints);
        std::string request = std::string("GET /ping HTTP/1.1\r\n") +
                              "Host: " + this->host + ":" + this->port + "\r\n" +
                              "Authorization: Token " + this->token + "\r\n" +
                              "Connection: close\r\n\r\n";
        boost::asio::write(socket, boost::asio::buffer(request));
        boost::asio::streambuf response;
        boost::system::error_code error;
        boost::asio::read(socket, response, boost::asio::transfer_all(), error);
        if (error != boost::asio::error::eof) {
            throw boost::system::system_error(error);
        }
        std::istream response_stream(&response);
        std::string http_version;
        unsigned int status_code;
        std::string status_message;
        response_stream >> http_version >> status_code;
        std::getline(response_stream, status_message);
        if (!response_stream || http_version.substr(0, 5) != "HTTP/") {
            std::cerr << "Invalid HTTP response.\n";
            return false;
        }
        if (status_code == 204) {
            return true; 
        } else {
            std::cerr << "Unexpected response status: " << status_code << " " << status_message << "\n";
            return false;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in connectedToInfluxDB: " << e.what() << "\n";
        return false;
    }
}
bool InfluxdbClient::postToInfluxdb(const std::string& bucket, const std::string& data, const std::string& precision = "ms") {
    try {
        boost::asio::io_context io_context;
        tcp::resolver resolver(io_context);
        tcp::resolver::results_type endpoints = resolver.resolve(this->host, this->port);
        tcp::socket socket(io_context);
        boost::asio::connect(socket, endpoints);
        std::string request = "POST /api/v2/write?org=" + this->org +
                              "&bucket=" + bucket + "&precision=" + precision + " HTTP/1.1\r\n" +
                              "Host: " + this->host + ":" + this->port + "\r\n" +
                              "Authorization: Token " + this->token + "\r\n" +
                              "Content-Type: text/plain; charset=utf-8\r\n" +
                              "Content-Length: " + std::to_string(data.length()) + "\r\n" +
                              "Connection: close\r\n\r\n" + data;
        //printf("%s\n", request.c_str());
        boost::asio::write(socket, boost::asio::buffer(request));
        boost::asio::streambuf response;
        boost::system::error_code error;
        boost::asio::read(socket, response, boost::asio::transfer_all(), error);
        if (error != boost::asio::error::eof) {
            throw boost::system::system_error(error);
        }
        std::istream response_stream(&response);
        std::string response_str((std::istreambuf_iterator<char>(response_stream)),
                                 std::istreambuf_iterator<char>());
        std::cout << "Response:\n" << response_str << std::endl;
        return true;
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return false;
    }
    return false;
}
static std::string FormatJsonValue(const nlohmann::json& val) {
    if (val.is_boolean()) {
        return val.get<bool>() ? "true" : "false";
    }
    else if (val.is_number_float()) {
        double d = val.get<double>();
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(9) << d;
        return oss.str();
    }
    else if (val.is_number_integer() || val.is_number_unsigned()) {
        return std::to_string(val.get<long long>());
    }
    else {
        // Unsupported type
        return "";
    }
}
bool InfluxdbClient::writeJsonToInfluxdb(const json& j, size_t batch_size = 1000) {
    // Validate JSON structure
    if (!j.contains("messages") || !j["messages"].is_array()) {
        std::cerr << "JSON does not contain a 'messages' array.\n";
        return false;
    }

    const auto& messages = j["messages"];
    if (messages.empty()) {
        std::cerr << "No messages to write.\n";
        return false;
    }

    // Get current system time in microseconds
    auto now = std::chrono::system_clock::now().time_since_epoch();
    long long current_timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(now).count();

    std::ostringstream batch_stream;
    size_t count = 0;

    for (const auto& msg : messages) {
        // Validate each message
        if (!msg.contains("fields") || !msg["fields"].is_object()) {
            std::cerr << "Message missing 'fields' object.\n";
            continue;
        }
        if (!msg.contains("measure") || !msg["measure"].is_string()) {
            std::cerr << "Message missing 'measure' string.\n";
            continue;
        }
        if (!msg.contains("timestamp") || !msg["timestamp"].is_number()) {
            std::cerr << "Message missing 'timestamp' field.\n";
            continue;
        }

        const std::string measurement = msg["measure"].get<std::string>();
        long long json_timestamp_us = msg["timestamp"].get<long long>();
        
        // Combine the JSON timestamp with the current system timestamp
        long long combined_timestamp_us = json_timestamp_us + current_timestamp_us;

        // Format:
        // measurement field1=val1,field2=val2 <combined_timestamp_us>u
        batch_stream << measurement << " ";

        // Serialize fields
        const auto& fields = msg["fields"];
        bool first_field = true;
        for (auto it = fields.begin(); it != fields.end(); ++it) {
            const std::string& field_name = it.key();
            const auto& field_value = it.value();
            std::string val_str = FormatJsonValue(field_value);
            if (val_str.empty()) {
                // Unsupported field type, skip
                std::cerr << "Unsupported field type for field '" << field_name << "'. Skipping.\n";
                continue;
            }
            if (!first_field) {
                batch_stream << ",";
            }
            batch_stream << field_name << "=" << val_str;
            first_field = false;
        }

        // Add the InfluxDB timestamp in microseconds with precision indicator 'u'
        batch_stream << " " << combined_timestamp_us << "\n"; // 'u' indicates microseconds

        // If we've reached the batch size, send this batch to InfluxDB
        if (++count >= batch_size) {
            std::string batch_data = batch_stream.str();
            if (!batch_data.empty()) {
                bool success = this->postToInfluxdb("test_2", batch_data, "us");
                if (!success) {
                    std::cerr << "Failed to post batch data to InfluxDB.\n";
                    return false;
                }
            }
            batch_stream.str("");  // Clear the stream for the next batch
            count = 0;  // Reset counter
        }
    }

    // Send any remaining data if there's any left after the loop
    std::string remaining_data = batch_stream.str();
    if (!remaining_data.empty()) {
        bool success = this->postToInfluxdb("test_2", remaining_data, "us");
        if (!success) {
            std::cerr << "Failed to post remaining data to InfluxDB.\n";
            return false;
        }
    }

    return true;
}
