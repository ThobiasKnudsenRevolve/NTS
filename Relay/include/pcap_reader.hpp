// PcapReader.hpp
#pragma once

#include <string>
#include <nlohmann/json.hpp>

nlohmann::json convertPcapFileToJson(const std::string& pcap_file_path);