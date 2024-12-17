// PcapReader.cpp
#include "pcap_reader.hpp"
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include "pcapplusplus/PcapFileDevice.h"
#include "pcapplusplus/Packet.h"
#include "pcapplusplus/ProtocolType.h"
#include "pcapplusplus/UdpLayer.h"

#include "udp_can_converter.hpp"

using json = nlohmann::json;

json convertPcapFileToJson(const std::string& pcap_file_path)
{
    UdpCanConverter converter;

    // Check for .pcap extension
    if (pcap_file_path.size() < 5 || pcap_file_path.substr(pcap_file_path.size() - 5) != ".pcap") {
        std::cerr << pcap_file_path << " does not have a .pcap extension." << std::endl;
        return "";
    }

    std::unique_ptr<pcpp::IFileReaderDevice> reader(pcpp::IFileReaderDevice::getReader(pcap_file_path.c_str()));

    if (!reader || !reader->open()) {
        std::cerr << "Error opening the pcap file: " << pcap_file_path << std::endl;
        return "";
    }

    // We'll accumulate all UDP payload data in a single buffer
    std::vector<uint8_t> accumulated_data;

    pcpp::RawPacket raw_packet;
    while (true) {
        if (!reader->getNextPacket(raw_packet)) {
            // No more packets
            break;
        }

        pcpp::Packet parsedPacket(&raw_packet, false, pcpp::UDP);
        pcpp::UdpLayer* udp_layer = parsedPacket.getLayerOfType<pcpp::UdpLayer>();
        if (udp_layer == nullptr) {
            continue;
        }

        const uint8_t* payload = udp_layer->getLayerPayload();
        size_t payloadLength = udp_layer->getLayerPayloadSize();

        // Append this UDP payload to our accumulated buffer
        size_t oldSize = accumulated_data.size();
        accumulated_data.resize(oldSize + payloadLength);
        std::copy_n(payload, payloadLength, accumulated_data.begin() + oldSize);
    }

    reader->close();

    // Now that we have all UDP payloads combined, convert them to JSON using the converter.
    if (!accumulated_data.empty()) {
        return converter.convertUdpCanToJson(accumulated_data.data(), accumulated_data.size());
    } else {
        // No data processed
        return "";
    }
}
