
#include "influxdb_client.hpp"
#include "channel.hpp"

#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <vector>

using boost::asio::ip::tcp;

static std::string FormatValue(const ChannelValue_t& val) {
    return std::visit([](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, bool>)
            return arg ? "true" : "false";
        else
            return std::to_string(arg);
    }, val);
}

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
bool InfluxdbClient::postToInfluxdb(const std::string& bucket, const std::string& data) {
    try {
        boost::asio::io_context io_context;
        tcp::resolver resolver(io_context);
        tcp::resolver::results_type endpoints = resolver.resolve(this->host, this->port);
        tcp::socket socket(io_context);
        boost::asio::connect(socket, endpoints);
        std::string request = "POST /api/v2/write?org=" + this->org +
                              "&bucket=" + bucket + "&precision=ms HTTP/1.1\r\n" +
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
void InfluxdbClient::writeChannelToInfluxdb(Channel& channel) {
    // Ensure channel is prepared
    // If Channel is const and we need to prepare, we must remove const or ensure it's already prepared.
    // Let's assume we can call a const method `isDataPrepared()`:
    if (!channel.isDataSorted()) {
        std::cout << "Channel is not prepared. preparing data before writing to InfluxDB.\n";
        channel.sortData();
        return;
    }

    // Get necessary data via public methods
    auto tags = channel.getTags();
    auto times = channel.getTimes();
    auto values = channel.getValues();
    std::string log_id = channel.getLogId();
    time_t posix_time = channel.getTimeForLogId();
    // Convert to ms
    long long base_millis = static_cast<long long>(posix_time) * 1000LL;

    if (times.empty() || values.empty()) {
        std::cerr << "Channel is empty\n";
        return;
    }

    // Get measurement and field via their public methods
    std::string measurement;
    std::string field;
    try {
        measurement = channel.getMeasurement();
        field = channel.getField();
    } catch (const std::out_of_range&) {
        std::cerr << "Channel does not have 'measure' or 'field' tags.\n";
        return;
    }

    // Other optional tags
    std::string car = (tags.find("car") != tags.end()) ? tags.at("car") : "";
    std::string driver = (tags.find("driver") != tags.end()) ? tags.at("driver") : "";
    std::string event_ = (tags.find("event") != tags.end()) ? tags.at("event") : "";
    std::string competition = (tags.find("competition") != tags.end()) ? tags.at("competition") : "";
    std::string log_type = (tags.find("log_type") != tags.end()) ? tags.at("log_type") : "";

    // Prepare line protocol data
    // We'll split into batches if needed
    size_t batch_size = 5000;
    size_t total_points = times.size();
    size_t num_batches = (total_points + batch_size - 1) / batch_size;

    for (size_t batch = 0; batch < num_batches; ++batch) {
        size_t start_idx = batch * batch_size;
        size_t end_idx = std::min(start_idx + batch_size, total_points);

        std::ostringstream oss;
        for (size_t i = start_idx; i < end_idx; ++i) {
            // Example line format: measurement,tagkey=tagvalue fieldkey=fieldvalue timestamp
            oss << measurement;
            oss << ",log_id=" << log_id; // log_id always present?
            if (!car.empty())        oss << ",car=" << car;
            if (!driver.empty())     oss << ",driver=" << driver;
            if (!event_.empty())     oss << ",event=" << event_;
            if (!competition.empty())oss << ",competition=" << competition;
            if (!log_type.empty())   oss << ",log_type=" << log_type;
            oss << " ";

            // field=value
            oss << field << "=" << FormatValue(values.at(i)) << " ";

            // timestamp in ms. times[i] presumably in ms or some unit?
            // If times[i] is in ms, just add base_millis + times[i].
            // If times[i] is in microseconds, convert accordingly.
            // The snippet says `static_cast<long long>(channel.m_time[i]/1000.f)`.
            // This suggests channel.m_time might be in microseconds.
            // Let's assume channel times are microseconds and we convert to ms:
            long long t_ms = base_millis + static_cast<long long>(times[i] / 1000.0);
            oss << t_ms << "\n";
        }

        std::string data = oss.str();
        // Post data
        if (!this->postToInfluxdb("CAN_Car", data)) {
            std::cerr << "Failed to post batch to InfluxDB.\n";
        }
    }
}
static std::string FormatJsonValue(const nlohmann::json& val) {
    // val is assumed to be numeric (float/int). If not, adjust accordingly.
    // For floats, we might want to ensure a certain precision.
    if (val.is_number_float()) {
        double d = val.get<double>();
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(9) << d;
        return oss.str();
    } else if (val.is_number_integer() || val.is_number_unsigned()) {
        return std::to_string(val.get<long long>());
    } else {
        // If a different type is encountered (like bool), handle it:
        if (val.is_boolean()) {
            return val.get<bool>() ? "true" : "false";
        }
        // For strings or other types, you may decide to skip or convert them:
        return ""; // or handle differently
    }
}

bool InfluxdbClient::writeJsonToInfluxdb(const nlohmann::json& j) {
    // Check if "messages" array exists
    if (!j.contains("messages") || !j["messages"].is_array()) {
        std::cerr << "JSON does not contain a 'messages' array.\n";
        return false;
    }

    const auto& messages = j["messages"];
    if (messages.empty()) {
        std::cerr << "No messages to write.\n";
        return false;
    }

    // We'll accumulate all lines into one payload
    std::ostringstream data_stream;

    // Current timestamp (in ns) for line protocol if none is provided
    // The user-provided JSON does not include timestamp, so we must pick one.
    // Influx requires timestamps in ns by default if not specified.
    // We'll just use current system time in nanoseconds.
    auto now = std::chrono::system_clock::now().time_since_epoch();
    long long timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();

    for (const auto& msg : messages) {
        // Each message should have "fields" and "measure"
        if (!msg.contains("fields") || !msg["fields"].is_object()) {
            std::cerr << "Message missing 'fields' object.\n";
            continue;
        }
        if (!msg.contains("measure") || !msg["measure"].is_string()) {
            std::cerr << "Message missing 'measure' string.\n";
            continue;
        }

        const std::string measurement = msg["measure"].get<std::string>();

        // Port_id as a tag
        int port_id = 0;
        if (msg.contains("port_id") && msg["port_id"].is_number()) {
            port_id = msg["port_id"].get<int>();
        }

        // Construct the line protocol:
        // measurement,port_id=<port_id> field1=val1,field2=val2 timestamp
        data_stream << measurement << ",port_id=" << port_id << " ";

        // Convert fields
        const auto& fields = msg["fields"];
        bool first_field = true;
        for (auto it = fields.begin(); it != fields.end(); ++it) {
            const std::string& field_name = it.key();
            const auto& field_value = it.value();
            std::string val_str = FormatJsonValue(field_value);
            if (val_str.empty()) {
                // If we cannot format a value, skip it.
                continue;
            }

            if (!first_field) {
                data_stream << ",";
            }
            data_stream << field_name << "=" << val_str;
            first_field = false;
        }

        // Add a space then timestamp
        // If you have a real timestamp from JSON, use that here instead of `timestamp_ns`.
        data_stream << " " << timestamp_ns << "\n";
    }

    std::string data = data_stream.str();
    if (data.empty()) {
        std::cerr << "No valid data lines were generated.\n";
        return false;
    }

    // Now post the data to InfluxDB
    // Assume "CAN_Car" is the bucket or adjust as needed
    bool success = this->postToInfluxdb("CAN_Car", data);
    if (!success) {
        std::cerr << "Failed to post data to InfluxDB.\n";
        return false;
    }

    return true;
}