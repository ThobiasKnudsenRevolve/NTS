#pragma once

#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <functional>
#include <chrono> // Include for timing
#include "libcanard/canard.h"

//#define DEBUG
#include "debug.hpp"

using namespace std;

// Memory allocation functions
void* memoryAllocate(CanardInstance* const ins, const size_t amount) {
    (void)ins;
    return malloc(amount);
}

void memoryFree(CanardInstance* const ins, void* const pointer) {
    (void)ins;
    free(pointer);
}

// struct to parse CAN ID
struct CanIdFields {
    CanardPriority priority;
    bool is_service_not_message;
    bool is_anonymous;
    CanardTransferKind transfer_kind;
    CanardPortID port_id;  // Subject ID or Service ID
    CanardNodeID source_node_id;
    CanardNodeID destination_node_id;  // For services
};

// Function to parse CAN ID
CanIdFields parseCanId(uint32_t can_id) {
    CanIdFields fields;

    fields.priority = (CanardPriority)((can_id >> 26U) & 0x7U);
    fields.is_service_not_message = ((can_id >> 25U) & 0x1U) != 0;
    fields.is_anonymous = (!fields.is_service_not_message) && (((can_id >> 24U) & 0x1U) != 0);
    fields.source_node_id = can_id & 0x7FU;  // Bits 6-0

    if (!fields.is_service_not_message) {
        // Message frame
        fields.transfer_kind = CanardTransferKindMessage;
        fields.port_id = (can_id >> 8U) & 0x1FFFU;  // Bits 23-8
        fields.destination_node_id = CANARD_NODE_ID_UNSET;  // Not applicable for messages
    } else {
        // Service frame
        fields.transfer_kind = ((can_id >> 24U) & 0x1U) ? CanardTransferKindRequest : CanardTransferKindResponse;
        fields.port_id = (can_id >> 14U) & 0x3FFU;  // Bits 23-14
        fields.destination_node_id = (can_id >> 7U) & 0x7FU;  // Bits 13-7
    }

    return fields;
}

// from autonomous pipeline but processAfterBeingReadFromSocket is changed
// https://github.com/RevolveNTNU/autonomous_pipeline/blob/master/subsystems/vehicle_interface/rdv_vehicle_interface_base/include/rdv_vehicle_interface_base/udp_socket.h
struct CanOverUdpMsg {
    uint64_t    timestamp;
    uint32_t    can_id;
    uint8_t     data_length;
    uint8_t     data[64]; // Adjust size as needed

    static constexpr size_t kHeaderLength = sizeof(timestamp) + sizeof(can_id) + sizeof(data_length);

    void processAfterBeingReadFromSocket(void) {
        timestamp = be64toh(timestamp);
        can_id = be32toh(can_id) & 0x1FFFFFFF; // Keep lower 29 bits
    }
};

// From autonomous pipeline but changed
// https://github.com/RevolveNTNU/autonomous_pipeline/blob/master/subsystems/vehicle_interface/rdv_vehicle_interface_base/include/rdv_vehicle_interface_base/udp_socket.h
bool readSingleMsg(const uint8_t* buffer, size_t buffer_size, size_t& offset, CanOverUdpMsg& msg) {
    if (offset + CanOverUdpMsg::kHeaderLength > buffer_size) {
        //printf("Insufficient data for CanOverUdpMsg at offset %ld\n", offset);
        return false;
    }

    // Deserialize header fields
    std::memcpy(&msg.timestamp, buffer + offset, sizeof(msg.timestamp));
    offset += sizeof(msg.timestamp);

    std::memcpy(&msg.can_id, buffer + offset, sizeof(msg.can_id));
    offset += sizeof(msg.can_id);

    msg.data_length = buffer[offset];
    offset += sizeof(msg.data_length);

    // Validate data_length
    if (msg.data_length > sizeof(msg.data)) {
        //printf("data_length (%d) exceeds maximum allowed size (%ld)\n", static_cast<int>(msg.data_length), sizeof(msg.data));
        return false;
    }

    // Ensure there is enough data left for the payload
    if (offset + msg.data_length > buffer_size) {
        //printf("Insufficient data for payload at offset %ld.\n", offset);
        return false;
    }

    // Deserialize payload
    if (msg.data_length > 0) {
        std::memcpy(msg.data, buffer + offset, msg.data_length);
    }
    offset += msg.data_length;

    // Process endianness and masking
    msg.processAfterBeingReadFromSocket();

    return true;
}

// look at these for better performance. you have to add all subscriptions before parsing
/*
        // Initialization phase
    void initializeSubscriptions(CanardInstance* ins) {
        static CanardRxSubscription subscription_array[MAX_SUBSCRIPTIONS];
        for (size_t i = 0; i < NUM_KNOWN_PORT_IDS; ++i) {
            int8_t res = canardRxSubscribe(ins,
                                           CanardTransferKindMessage,
                                           known_port_ids[i],
                                           MAX_EXTENT,
                                           TRANSFER_ID_TIMEOUT_USEC,
                                           &subscription_array[i]);
            if (res < 0) {
                // Handle error
            }
        }
    }

    // Main processing loop
    void processFrames(CanardInstance* ins, const CanardFrame* frame) {
        CanardRxTransfer transfer;
        int8_t result = canardRxAccept(ins, timestamp, frame, 0, &transfer, NULL);
        if (result > 0) {
            // Process the transfer
            processTransfer(&transfer);
            // Release the payload
            ins->memory_free(ins, (void*)transfer.payload);
        }
    }    

*/


bool extractUdpMsg(
    const uint8_t* payload, 
    size_t payload_length, 
    size_t& offset, 
    CanardInstance& canard_instance,
    DataManager& data_manager,
    DeserializationMap& deserialization_map) 
{
    CanOverUdpMsg udp_msg;

    if (readSingleMsg(payload, payload_length, offset, udp_msg)) {
        CanardFrame frame;
        frame.extended_can_id = udp_msg.can_id;
        frame.payload_size = udp_msg.data_length;
        frame.payload = udp_msg.data;

        // Parse CAN ID
        CanIdFields can_id_fields = parseCanId(frame.extended_can_id);

        // Get or create subscription
        CanardRxSubscription* subscription = nullptr;
        int8_t sub_result = canardRxGetSubscription(&canard_instance, can_id_fields.transfer_kind, can_id_fields.port_id, &subscription);

        if (sub_result <= 0) {
            // Subscription does not exist; create it
            CanardRxSubscription* new_subscription = (CanardRxSubscription*)malloc(sizeof(CanardRxSubscription));
            size_t extent = 1024;  // Use the maximum extent or adjust as needed
            CanardMicrosecond transfer_id_timeout_usec = CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC;
            int8_t res = canardRxSubscribe(&canard_instance, can_id_fields.transfer_kind, can_id_fields.port_id, extent, transfer_id_timeout_usec, new_subscription);

            if (res < 0) {
                printf("Subscription failed with error: %d\n", static_cast<int>(res));
                free(new_subscription);
                return false;
            }
            subscription = new_subscription;
        }

        // Accept the frame
        CanardRxTransfer transfer;
        int8_t result = canardRxAccept(&canard_instance, udp_msg.timestamp, &frame, 0, &transfer, &subscription);

        if (result == 1) {
            // Deserialization
            auto it = deserialization_map.find(transfer.metadata.port_id);
            if (it != deserialization_map.end()) {
                //printf("Deserializing port ID: %d\n", transfer.metadata.port_id);
                it->second(data_manager, transfer);
            } else {
                //printf("No deserialization function for port ID: %d\n", transfer.metadata.port_id);
            }
            canard_instance.memory_free(&canard_instance, (void*)transfer.payload);
        } else if (result < 0) {
            printf("Reception error: %d", static_cast<int>(result));
            return false;
        } else {
            // Frame accepted but transfer not yet complete
        }
    } else {
        printf("Failed to deserialize CanOverUdpMsg at offset %ld.\n", offset);
        return false;
    }

    return true;
}
