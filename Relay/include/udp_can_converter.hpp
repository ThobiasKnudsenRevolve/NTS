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

class UdpCanConverter {
public:
    UdpCanConverter();
    ~UdpCanConverter();

    nlohmann::json convertUdpCanToJson(void* data, size_t size);
    std::vector<uint8_t> convertJsonToUdpCan(const nlohmann::json& j);

private:
    struct CanOverUdpMessage {
        uint64_t timestamp;
        uint32_t canId;
        uint8_t  dataLength;
        uint8_t  data[64]; 
        static constexpr size_t headerLength = sizeof(timestamp) + sizeof(canId) + sizeof(dataLength);
    };

    struct CanIdentifierFields {
        CanardPriority priority;
        bool isService;
        bool isAnonymous;
        CanardTransferKind transferKind;
        CanardPortID portId;
        CanardNodeID sourceNodeId;
        CanardNodeID destinationNodeId;
    };

    // Internal utility methods
    bool readSingleCanOverUdpMessage(const uint8_t* buffer, size_t bufferSize, size_t& offset, CanOverUdpMessage& message);
    CanIdentifierFields parseCanId(uint32_t canId);

    bool deserializeToJson(CanardPortID portId, const CanardRxTransfer& transfer, nlohmann::json& messageJson);

    static void* canardMemoryAllocate(CanardInstance* const, const size_t amount);
    static void canardMemoryFree(CanardInstance* const, void* const pointer);

    // Endianness helpers
    static uint64_t fromBigEndian64(uint64_t value);
    static uint32_t fromBigEndian32(uint32_t value);
    static uint64_t toBigEndian64(uint64_t value);
    static uint32_t toBigEndian32(uint32_t value);

    // Canard instance and subscriptions
    CanardInstance m_canard_instance;
    std::unordered_map<CanardPortID, CanardRxSubscription*> m_subscriptions;
};

