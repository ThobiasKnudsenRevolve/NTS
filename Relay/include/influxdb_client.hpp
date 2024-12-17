// influxdb_client.hpp
#pragma once

#include <string>
#include <nlohmann/json.hpp>

class InfluxdbClient {
private:
    std::string host;
    std::string port;
    std::string org;
    std::string token;

public:
    InfluxdbClient(const std::string& host, const std::string& port, const std::string& org, const std::string& token);
    ~InfluxdbClient();

    bool isConnectedToInfluxdb() const;
    bool postToInfluxdb(const std::string& bucket, const std::string& data, const std::string& precision);
    bool writeJsonToInfluxdb(const nlohmann::json& j, size_t batch_size);

    // Delete copy and move constructors and assignment operators
    InfluxdbClient(const InfluxdbClient&) = delete;
    InfluxdbClient& operator=(const InfluxdbClient&) = delete;
    InfluxdbClient(InfluxdbClient&&) = delete;
    InfluxdbClient& operator=(InfluxdbClient&&) = delete;
};
