#pragma once

#include <iostream>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <iomanip>
#include <arpa/inet.h>
#include "pcapplusplus/PcapFileDevice.h"
#include "pcapplusplus/Packet.h"
#include "pcapplusplus/ProtocolType.h"
#include "pcapplusplus/UdpLayer.h"
#include "pcapplusplus/PointerVector.h"

#include "Deserialization.hpp"
#include "ExtractUdpMsg.hpp"

//#define DEBUG
#include "debug.hpp"

int readPcapFileToDataManager(
    const std::string& pcap_file_path,
    DataManager& uber_data_manager,
    DeserializationMap& deserialization_map,
    std::atomic<bool>& stop_flag)
{
    // Check if the file ends with ".pcap"
    if (pcap_file_path.rfind(".pcap") != (pcap_file_path.size() - 5)) {
        printf("%s does not end with .pcap\n", pcap_file_path.c_str());
        exit(-1);
    }

    // Initialize Canard instance
    CanardInstance canard_instance = canardInit(memoryAllocate, memoryFree);
    canard_instance.node_id = CANARD_NODE_ID_UNSET;

    // Open the pcap file
    pcpp::IFileReaderDevice* reader = pcpp::IFileReaderDevice::getReader(pcap_file_path.c_str());
    if (!reader->open()) {
        std::cerr << "Error opening the pcap file" << std::endl;
        return 1;
    }

    DataManager sub_data_manager;

    // Counters
    size_t packet_count = 0;
    size_t udp_packet_count = 0;

    pcpp::RawPacket rawPacket;
    while (true) {
        if (stop_flag.load())
            break;
        bool hasPacket = reader->getNextPacket(rawPacket);
        if (!hasPacket)
            break;
        packet_count++;
        pcpp::Packet parsedPacket(&rawPacket, false, pcpp::UDP);
        pcpp::UdpLayer* udpLayer = parsedPacket.getLayerOfType<pcpp::UdpLayer>();
        if (udpLayer == nullptr)
            continue;
        udp_packet_count++;
        const uint8_t* payload = udpLayer->getLayerPayload();
        size_t payloadLength = udpLayer->getLayerPayloadSize();
        size_t offset = 0;
        while (offset < payloadLength) {
            if (!extractUdpMsg(payload, payloadLength, offset, canard_instance, sub_data_manager, deserialization_map)) {
                break;
            }
        }
    }

    Channel<float>* channel_ptr = uber_data_manager.GetChannelPtr<float>("CAN_2024-11-20(142000)", "vcu", "INS.vx");
    if (!channel_ptr) {printf("could not find channel\n");} 
    else {printf("size=%ld\n", channel_ptr->m_value.size());}
    uber_data_manager += sub_data_manager;
    channel_ptr = uber_data_manager.GetChannelPtr<float>("CAN_2024-11-20(142000)", "vcu", "INS.vx");
    if (!channel_ptr) {printf("could not find channel\n");} 
    else {printf("size=%ld\n", channel_ptr->m_value.size());}
    reader->close();
    delete reader;

    return 0;
}
