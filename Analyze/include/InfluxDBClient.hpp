#pragma once

#include <boost/asio.hpp>
#include <iostream>
#include <string>

using boost::asio::ip::tcp;

class InfluxDBClient {

private:

    std::string host;
    std::string port;
    std::string org;
    std::string token;

    std::vector<std::string> cars = {"Hera", "Lyra"};
    std::vector<std::string> competitions = {"FSEast", "FSG"};
    std::vector<std::string> drivers = {"Driverless", "Stian Persson Lie", "Stian Lie", "Håvard V. Hagerup", "Håvard Hagerup", "Mads Engesvoll", "Magnus Husby", "Eivind Due-Tønnesen", "Other", "Not Relevant"};
    std::vector<std::string> events = {"Autocross", "Endurance", "Training", "Acceleration", "Skidpad", "Brake Test", "BrakeTest", "Shakedown", "Trackdrive", "Other"};
    std::vector<std::string> laps = {"ToGarage", "ToStart", "InPits", "%d"};
    std::vector<std::string> log_type = {"TEST_DRIVE", "COMPETITION_DRIVE"};

public:

    InfluxDBClient(const std::string& host, const std::string& port, const std::string& org, const std::string& token): host(host), port(port), org(org), token(token) {}
    ~InfluxDBClient() {}

    void postToInfluxDB(const std::string& bucket,
                         const std::string& data) {
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
            printf("%s\n", request.c_str());
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
        }
        catch (std::exception& e) {
            std::cerr << "Exception: " << e.what() << "\n";
        }
    }

};
