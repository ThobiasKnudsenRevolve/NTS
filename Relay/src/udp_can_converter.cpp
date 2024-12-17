#include "udp_can_converter.hpp"
using json = nlohmann::json;

UdpCanConverter::UdpCanConverter() {
    m_canard_instance = canardInit(canardMemoryAllocate, canardMemoryFree);
    m_canard_instance.node_id = CANARD_NODE_ID_UNSET;
}
UdpCanConverter::~UdpCanConverter() {
    for (auto& [portId, subscription] : m_subscriptions) {
        delete subscription;
    }
}
void* UdpCanConverter::canardMemoryAllocate(CanardInstance*, const size_t amount) {
    return new uint8_t[amount];
}
void UdpCanConverter::canardMemoryFree(CanardInstance*, void* const pointer) {
    auto ptr = static_cast<uint8_t*>(pointer);
    delete[] ptr;
}
uint64_t UdpCanConverter::fromBigEndian64(uint64_t value) {
    const uint64_t b0 = (value & 0x00000000000000FFULL) << 56U;
    const uint64_t b1 = (value & 0x000000000000FF00ULL) << 40U;
    const uint64_t b2 = (value & 0x0000000000FF0000ULL) << 24U;
    const uint64_t b3 = (value & 0x00000000FF000000ULL) << 8U;
    const uint64_t b4 = (value & 0x000000FF00000000ULL) >> 8U;
    const uint64_t b5 = (value & 0x0000FF0000000000ULL) >> 24U;
    const uint64_t b6 = (value & 0x00FF000000000000ULL) >> 40U;
    const uint64_t b7 = (value & 0xFF00000000000000ULL) >> 56U;
    return b0 | b1 | b2 | b3 | b4 | b5 | b6 | b7;
}
uint32_t UdpCanConverter::fromBigEndian32(uint32_t value) {
    const uint32_t b0 = (value & 0x000000FFUL) << 24U;
    const uint32_t b1 = (value & 0x0000FF00UL) << 8U;
    const uint32_t b2 = (value & 0x00FF0000UL) >> 8U;
    const uint32_t b3 = (value & 0xFF000000UL) >> 24U;
    return b0 | b1 | b2 | b3;
}
uint64_t UdpCanConverter::toBigEndian64(uint64_t value) {
    return fromBigEndian64(value); 
}
uint32_t UdpCanConverter::toBigEndian32(uint32_t value) {
    return fromBigEndian32(value);
}
bool UdpCanConverter::readSingleCanOverUdpMessage(const uint8_t* buffer, size_t bufferSize, size_t& offset, CanOverUdpMessage& message) {
    if (offset + CanOverUdpMessage::headerLength > bufferSize) {
        return false;
    }

    uint64_t rawTimestamp = 0;
    std::copy_n(buffer + offset, sizeof(rawTimestamp), reinterpret_cast<uint8_t*>(&rawTimestamp));
    message.timestamp = fromBigEndian64(rawTimestamp);
    offset += sizeof(message.timestamp);

    uint32_t rawCanId = 0;
    std::copy_n(buffer + offset, sizeof(rawCanId), reinterpret_cast<uint8_t*>(&rawCanId));
    message.canId = fromBigEndian32(rawCanId) & 0x1FFFFFFF;
    offset += sizeof(message.canId);

    message.dataLength = buffer[offset];
    offset += sizeof(message.dataLength);

    if (message.dataLength > sizeof(message.data)) {
        return false;
    }

    if (offset + message.dataLength > bufferSize) {
        return false;
    }

    if (message.dataLength > 0) {
        std::copy_n(buffer + offset, message.dataLength, message.data);
    }
    offset += message.dataLength;

    return true;
}
UdpCanConverter::CanIdentifierFields UdpCanConverter::parseCanId(uint32_t canId) {
    CanIdentifierFields fields;
    fields.priority = static_cast<CanardPriority>((canId >> 26U) & 0x7U);
    fields.isService = ((canId >> 25U) & 0x1U) != 0;
    fields.isAnonymous = (!fields.isService) && (((canId >> 24U) & 0x1U) != 0);
    fields.sourceNodeId = canId & 0x7FU;

    if (!fields.isService) {
        fields.transferKind = CanardTransferKindMessage;
        fields.portId = (canId >> 8U) & 0x1FFFU;
        fields.destinationNodeId = CANARD_NODE_ID_UNSET;
    } else {
        fields.transferKind = ((canId >> 24U) & 0x1U) ? CanardTransferKindRequest : CanardTransferKindResponse;
        fields.portId = (canId >> 14U) & 0x3FFU;
        fields.destinationNodeId = (canId >> 7U) & 0x7FU;
    }

    return fields;
}
json UdpCanConverter::convertUdpCanToJson(void* data, size_t size) {
    const uint8_t* buffer = static_cast<const uint8_t*>(data);
    size_t offset = 0;
    json result_json;
    result_json["messages"] = json::array();

    while (offset < size) {
        CanOverUdpMessage msg;
        if (!readSingleCanOverUdpMessage(buffer, size, offset, msg)) {
            break;
        }

        auto fields = parseCanId(msg.canId);

        // Check or create subscription
        CanardRxSubscription* subscription = nullptr;
        int8_t getSubResult = canardRxGetSubscription(&m_canard_instance, fields.transferKind, fields.portId, &subscription);
        if (getSubResult <= 0) {
            auto newSubscription = new CanardRxSubscription;
            size_t extent = 1024;
            CanardMicrosecond transferIdTimeout = CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC;
            int8_t subscribeResult = canardRxSubscribe(
                &m_canard_instance, fields.transferKind, fields.portId, extent, transferIdTimeout, newSubscription
            );
            if (subscribeResult < 0) {
                delete newSubscription;
                std::cerr << "Subscription failed for port " << fields.portId << " with error: " << static_cast<int>(subscribeResult) << "\n";
                continue;
            }
            subscription = newSubscription;
            m_subscriptions[fields.portId] = subscription;
        }

        // Create a CanardFrame
        CanardFrame frame;
        frame.extended_can_id = msg.canId;
        frame.payload_size = msg.dataLength;
        frame.payload = msg.data;

        // Accept the frame
        CanardRxTransfer transfer;
        int8_t acceptResult = canardRxAccept(&m_canard_instance, msg.timestamp, &frame, 0, &transfer, &subscription);

        if (acceptResult == 1) {
            // Complete transfer
            json messageJson;
            if (deserializeToJson(transfer.metadata.port_id, transfer, messageJson)) {
                result_json["messages"].push_back(messageJson);
            }
            m_canard_instance.memory_free(&m_canard_instance, const_cast<void*>(transfer.payload));
        } else if (acceptResult < 0) {
            std::cerr << "Reception error: " << static_cast<int>(acceptResult) << "\n";
            // error
        }
        // if acceptResult == 0 => not complete yet, waiting for more frames
    }

    return result_json;
}
std::vector<uint8_t> UdpCanConverter::convertJsonToUdpCan(const json& j) {

    if (!j.contains("messages") || !j["messages"].is_array()) {
        std::cerr << "JSON does not contain 'messages' array.\n";
        return {};
    }

    std::vector<uint8_t> udpPayload;

    for (auto& msgEntry : j["messages"]) {
        if (!msgEntry.contains("timestamp") || !msgEntry.contains("can_id") || !msgEntry.contains("data")) {
            std::cerr << "Message entry missing required fields (timestamp, can_id, data).\n";
            continue;
        }

        uint64_t hostTimestamp = msgEntry["timestamp"].get<uint64_t>();
        uint32_t hostCanId = msgEntry["can_id"].get<uint32_t>();
        auto dataVec = msgEntry["data"].get<std::vector<uint8_t>>();

        if (dataVec.size() > 64) {
            std::cerr << "Data size too large for message.\n";
            continue;
        }

        uint64_t networkTimestamp = toBigEndian64(hostTimestamp);
        uint32_t networkCanId = toBigEndian32(hostCanId);

        size_t oldSize = udpPayload.size();
        udpPayload.resize(oldSize + CanOverUdpMessage::headerLength + dataVec.size());

        size_t offset = oldSize;

        auto tsPtr = reinterpret_cast<uint8_t*>(&networkTimestamp);
        std::copy_n(tsPtr, sizeof(networkTimestamp), udpPayload.begin() + offset);
        offset += sizeof(networkTimestamp);

        auto idPtr = reinterpret_cast<uint8_t*>(&networkCanId);
        std::copy_n(idPtr, sizeof(networkCanId), udpPayload.begin() + offset);
        offset += sizeof(networkCanId);

        udpPayload[offset] = static_cast<uint8_t>(dataVec.size());
        offset += sizeof(uint8_t);

        std::copy_n(dataVec.data(), dataVec.size(), udpPayload.begin() + offset);
        offset += dataVec.size();
    }

    return udpPayload;
}
