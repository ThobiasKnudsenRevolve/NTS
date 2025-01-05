#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <iostream>
#include <algorithm>
#include <stdexcept>

#include "libcanard/canard.h"

class Converter {
public:
    Converter();
    ~Converter();

    std::string jsonToFlux(const nlohmann::json& j);
    nlohmann::json pcapFileToJson(const std::string& pcap_file_path);
    nlohmann::json udpToJson(const uint8_t* const udp_msg, const size_t udp_msg_size);
    std::vector<uint8_t> jsonToCan(const nlohmann::json& j);
    std::vector<uint8_t> jsonToUdp(const nlohmann::json& j);

private:
    struct CanMessage {
        uint64_t timestamp;
        uint32_t can_id;
        uint8_t  data_length;
        uint8_t  data[64]; 
        static constexpr size_t header_length = sizeof(timestamp) + sizeof(can_id) + sizeof(data_length);
    };

    struct CanIdFields {
        CanardPriority priority;
        bool is_service;
        bool is_anonymous;
        CanardTransferKind transfer_kind;
        CanardPortID port_id;
        CanardNodeID source_node_id;
        CanardNodeID destination_node_id;
    };

    // Internal utility methods
    bool getNextCanMessageOverUdpMessage(const uint8_t* const can_msg, const size_t can_msg_size, size_t& can_msg_offset, CanMessage& message);
    CanIdFields deserializeCanId(const uint32_t can_id);

    bool deserializeDslsCanTransferToJson(const CanardPortID port_id, const CanardRxTransfer& transfer, nlohmann::json& msg_json);
    bool serializeJsonToDsdlCanPayload(const nlohmann::json& msg_json, void* const payload, size_t& payload_size);
    std::vector<std::vector<uint8_t>> createCanFramesFromCanPayload(const uint32_t port_id, const void* const payload, const size_t& payload_size);

    static void* canardMemoryAllocate(CanardInstance* const, const size_t amount);
    static void canardMemoryFree(CanardInstance* const, void* const pointer);

    // Endianness helpers
    static uint64_t toBigEndian64(uint64_t value);
    static uint32_t toBigEndian32(uint32_t value);

    // Canard instance and subscriptions
    CanardInstance m_canard_instance;
    std::unordered_map<CanardPortID, CanardRxSubscription*> m_subscriptions;
};
