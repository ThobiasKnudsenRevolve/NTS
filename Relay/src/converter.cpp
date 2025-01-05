#include "converter.hpp"

#include "pcap_reader.hpp"
#include "pcapplusplus/PcapFileDevice.h"
#include "pcapplusplus/Packet.h"
#include "pcapplusplus/ProtocolType.h"
#include "pcapplusplus/UdpLayer.h"

using json = nlohmann::json;

#include "vcu/EbsActivation_1_0.h"              // 8
#include "vcu/DriveMode_1_0.h"                  // 10
#include "vcu/Config_1_0.h"                     // 13
#include "vcu/INS_1_0.h"                        // 102
#include "vcu/EnergyMeter_1_0.h"                // 111
#include "vcu/DVStates_1_0.h"                   // 113
#include "vcu/InsEstimates1_1_0.h"              // 114 lat and lon is double and not float
#include "vcu/EPOSData_1_0.h"
#include "vcu/EposStatus_1_0.h"
#include "vcu/GetConfig_1_0.h"
#include "vcu/GNSS_1_0.h"
#include "vcu/Implausibilities_1_0.h"
#include "vcu/ImuMeasurements_1_0.h"
#include "vcu/InsEstimates2_1_0.h"
#include "vcu/InsStatus_1_0.h"
#include "vcu/InverterEstimates_1_0.h"
#include "vcu/KERS_1_0.h"
#include "vcu/MissionSelect_1_0.h"
#include "vcu/ResetInverter_1_0.h"
#include "vcu/RESStatus_1_0.h"
#include "vcu/SCS_1_0.h"
#include "vcu/State_1_0.h"
#include "vcu/StaticOutputs_1_0.h"
#include "vcu/Status_1_0.h"
#include "vcu/TelemetryStatus_1_0.h"
#include "vcu/TSABPressed_1_0.h"
#include "vcu/TVOutputs_1_0.h"
#include "vcu/TVStatus_1_0.h"
#include "vcu/VcuStatus_1_0.h"
#include "vcu/Warning_1_0.h"
#include "vcu/ZeroIns_1_0.h"


void* Converter::canardMemoryAllocate(CanardInstance*, const size_t amount) {
    return new uint8_t[amount];
}
void Converter::canardMemoryFree(CanardInstance*, void* const pointer) {
    auto ptr = static_cast<uint8_t*>(pointer);
    delete[] ptr;
}
Converter::Converter() {
    m_canard_instance = canardInit(canardMemoryAllocate, canardMemoryFree);
    m_canard_instance.node_id = CANARD_NODE_ID_UNSET;
}
Converter::~Converter() {
    for (auto& [portId, subscription] : m_subscriptions) {
        delete subscription;
    }
}
uint64_t Converter::toBigEndian64(uint64_t value) {
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
uint32_t Converter::toBigEndian32(uint32_t value) {
    const uint32_t b0 = (value & 0x000000FFUL) << 24U;
    const uint32_t b1 = (value & 0x0000FF00UL) << 8U;
    const uint32_t b2 = (value & 0x00FF0000UL) >> 8U;
    const uint32_t b3 = (value & 0xFF000000UL) >> 24U;
    return b0 | b1 | b2 | b3;
}

// FLUX =======================================================================================================================================
std::string jsonToFlux(const json& j) {

}

// PCAP =======================================================================================================================================
json Converter::pcapFileToJson(const std::string& pcap_file_path)
{
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

    std::vector<uint8_t> accumulated_data;

    pcpp::RawPacket raw_packet;
    while (true) {
        if (!reader->getNextPacket(raw_packet))
            break;

        pcpp::Packet parsedPacket(&raw_packet, false, pcpp::UDP);
        pcpp::UdpLayer* udp_layer = parsedPacket.getLayerOfType<pcpp::UdpLayer>();
        if (udp_layer == nullptr)
            continue;

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
        return udpToJson(accumulated_data.data(), accumulated_data.size());
    } else {
        return "";
    }
}


// DESERIALIZE ================================================================================================================================

bool Converter::getNextCanMessageOverUdpMessage(const uint8_t* const udp_msg, const size_t udp_msg_size, size_t& udp_msg_offset, CanMessage& can_msg) {

    if (udp_msg == nullptr) {
        std::cerr << "udp_msg is nullptr\n";
        return false;
    }

    if (udp_msg_offset + CanMessage::header_length > udp_msg_size) {
        return false;
    }

    uint64_t raw_Timestamp = 0;
    std::copy_n(udp_msg + udp_msg_offset, sizeof(raw_Timestamp), reinterpret_cast<uint8_t*>(&raw_Timestamp));
    can_msg.timestamp = toBigEndian64(raw_Timestamp);
    udp_msg_offset += sizeof(can_msg.timestamp);

    uint32_t raw_can_id = 0;
    std::copy_n(udp_msg + udp_msg_offset, sizeof(raw_can_id), reinterpret_cast<uint8_t*>(&raw_can_id));
    can_msg.can_id = toBigEndian32(raw_can_id) & 0x1FFFFFFF;
    udp_msg_offset += sizeof(can_msg.can_id);

    can_msg.data_length = udp_msg[udp_msg_offset];
    udp_msg_offset += sizeof(can_msg.data_length);

    if (can_msg.data_length > sizeof(can_msg.data)) {
        std::cout << "payload data length is greater than " << sizeof(can_msg.data) << std::endl;
        return false;
    }
    if (udp_msg_offset + can_msg.data_length > udp_msg_size) {
        return false;
    }
    if (can_msg.data_length > 0) {
        std::copy_n(udp_msg + udp_msg_offset, can_msg.data_length, can_msg.data);
    }
    udp_msg_offset += can_msg.data_length;

    return true;
}
Converter::CanIdFields Converter::deserializeCanId(const uint32_t can_id) {
    CanIdFields fields;
    fields.priority = static_cast<CanardPriority>((can_id >> 26U) & 0x7U);
    fields.is_service = ((can_id >> 25U) & 0x1U) != 0;
    fields.is_anonymous = (!fields.is_service) && (((can_id >> 24U) & 0x1U) != 0);
    fields.source_node_id = can_id & 0x7FU;

    if (!fields.is_service) {
        fields.transfer_kind = CanardTransferKindMessage;
        fields.port_id = (can_id >> 8U) & 0x1FFFU;
        fields.destination_node_id = CANARD_NODE_ID_UNSET;
    } else {
        fields.transfer_kind = ((can_id >> 24U) & 0x1U) ? CanardTransferKindRequest : CanardTransferKindResponse;
        fields.port_id = (can_id >> 14U) & 0x3FFU;
        fields.destination_node_id = (can_id >> 7U) & 0x7FU;
    }

    return fields;
}
json Converter::udpToJson(const uint8_t* udp_msg, size_t udp_msg_size) {
    if (udp_msg == nullptr) {
        std::cerr << "Error: udp_msg pointer is nullptr\n";
        return nlohmann::json::object();
    }

    json result_json;
    result_json["messages"] = nlohmann::json::array();
    size_t offset = 0;
    
    while (offset < udp_msg_size) {
        CanMessage can_msg;
        if (!this->getNextCanMessageOverUdpMessage(udp_msg, udp_msg_size, offset, can_msg)) {
            std::cerr << "No further CAN messages can be parsed from UDP or parse error.\n";
            break;  // Stop processing
        }

        Converter::CanIdFields fields = this->deserializeCanId(can_msg.can_id);
        CanardRxSubscription* subscription = nullptr;
        const int8_t get_sub_result = canardRxGetSubscription(&m_canard_instance, fields.transfer_kind, fields.port_id, &subscription);

        if (get_sub_result <= 0) {
            auto new_subscription = new CanardRxSubscription;
            constexpr size_t          extent               = 256;  // adjust as needed
            constexpr CanardMicrosecond transfer_id_timeout = CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC;

            const int8_t subscribe_result = canardRxSubscribe(&m_canard_instance, fields.transfer_kind, fields.port_id, extent, transfer_id_timeout, new_subscription);

            if (subscribe_result < 0) {
                delete new_subscription;
                std::cerr << "Subscription failed for port " << fields.port_id << " with error: " << static_cast<int>(subscribe_result) << "\n";
                continue;
            }

            subscription = new_subscription;
            m_subscriptions[fields.port_id] = subscription;
        }

        // Prepare the Libcanard frame
        CanardFrame can_frame{
            .extended_can_id = can_msg.can_id,
            .payload_size    = can_msg.data_length,
            .payload         = can_msg.data
        };

        CanardRxTransfer transfer;
        const int8_t accept_result = canardRxAccept(&m_canard_instance, can_msg.timestamp, &can_frame, 0, &transfer, &subscription);

        if (accept_result == 1) {
            nlohmann::json msg_json;
            if (deserializeDslsCanTransferToJson(transfer.metadata.port_id, transfer, msg_json)) {
                msg_json["can_id"] = can_frame.extended_can_id;
                result_json["messages"].push_back(msg_json);
            }
            m_canard_instance.memory_free(&m_canard_instance, const_cast<void*>(transfer.payload));
        }
        else if (accept_result == 0) {
            // ?
        }
        else if (accept_result < 0) {
            std::cerr << "Reception error: " << static_cast<int>(accept_result) << "\n";
        }
    }

    return result_json;
}
bool Converter::deserializeDslsCanTransferToJson(const CanardPortID port_id, const CanardRxTransfer& transfer, nlohmann::json& msg_json) {

    msg_json["port_id"]   = port_id;
    msg_json["timestamp"] = transfer.timestamp_usec;
    msg_json["fields"]    = nlohmann::json::object();

    size_t payload_size = transfer.payload_size;
    int8_t deserialization_result = -1;

    try {
        switch (port_id) {
            case 8: // EbsActivation
            {
                vcu_EbsActivation_Request_1_0 obj;
                vcu_EbsActivation_Request_1_0_initialize_(&obj);
                deserialization_result = vcu_EbsActivation_Request_1_0_deserialize_( &obj, static_cast<const uint8_t*>(transfer.payload), &payload_size);
                if (deserialization_result >= 0) {
                    msg_json["measure"] = "vcu";
                    msg_json["fields"]["EbsActivation.activate_ebs"] = obj.activate_ebs;
                }
            }
            break;

            case 10: // DriveMode
            {
                vcu_DriveMode_Request_1_0 obj;
                vcu_DriveMode_Request_1_0_initialize_(&obj);
                deserialization_result = vcu_DriveMode_Request_1_0_deserialize_( &obj, static_cast<const uint8_t*>(transfer.payload), &payload_size);
                if (deserialization_result >= 0) {
                    msg_json["measure"] = "vcu";
                    msg_json["fields"]["DriveMode.command"] = obj.command;
                }
            }
            break;

            case 13: // Config
            {
                vcu_Config_Request_1_0 obj;
                vcu_Config_Request_1_0_initialize_(&obj);
                deserialization_result = vcu_Config_Request_1_0_deserialize_( &obj, static_cast<const uint8_t*>(transfer.payload), &payload_size);
                if (deserialization_result >= 0) {
                    msg_json["measure"] = "vcu";
                    msg_json["fields"]["Config.st_trq"]       = obj.st_trq;
                    msg_json["fields"]["Config.st_rpm"]       = obj.st_rpm;
                    msg_json["fields"]["Config.pitch_offset"] = obj.pitch_offset;
                    msg_json["fields"]["Config.roll_offset"]  = obj.roll_offset;
                }
            }
            break;

            case 102: // INS
            {
                vcu_INS_1_0 obj;
                vcu_INS_1_0_initialize_(&obj);
                deserialization_result = vcu_INS_1_0_deserialize_( &obj, static_cast<const uint8_t*>(transfer.payload), &payload_size);
                if (deserialization_result >= 0)
                {
                    msg_json["measure"] = "vcu";
                    msg_json["fields"]["INS.vx"]            = obj.vx;
                    msg_json["fields"]["INS.vy"]            = obj.vy;
                    msg_json["fields"]["INS.vz"]            = obj.vz;
                    msg_json["fields"]["INS.ax"]            = obj.ax;
                    msg_json["fields"]["INS.ay"]            = obj.ay;
                    msg_json["fields"]["INS.az"]            = obj.az;
                    msg_json["fields"]["INS.roll"]          = obj.roll;
                    msg_json["fields"]["INS.pitch"]         = obj.pitch;
                    msg_json["fields"]["INS.yaw"]           = obj.yaw;
                    msg_json["fields"]["INS.roll_rate"]     = obj.roll_rate;
                    msg_json["fields"]["INS.pitch_rate"]    = obj.pitch_rate;
                    msg_json["fields"]["INS.yaw_rate"]      = obj.yaw_rate;
                    msg_json["fields"]["INS.roll_rate_dt"]  = obj.roll_rate_dt;
                    msg_json["fields"]["INS.pitch_rate_dt"] = obj.pitch_rate_dt;
                    msg_json["fields"]["INS.yaw_rate_dt"]   = obj.yaw_rate_dt;
                }
            }
            break;

            case 111: // EnergyMeter
            {
                vcu_EnergyMeter_1_0 obj;
                vcu_EnergyMeter_1_0_initialize_(&obj);
                deserialization_result = vcu_EnergyMeter_1_0_deserialize_( &obj, static_cast<const uint8_t*>(transfer.payload), &payload_size);
                if (deserialization_result >= 0)
                {
                    msg_json["measure"] = "vcu";
                    msg_json["fields"]["EnergyMeter.counter"]           = obj.counter;
                    msg_json["fields"]["EnergyMeter.ready"]             = obj.ready;
                    msg_json["fields"]["EnergyMeter.logging"]           = obj.logging;
                    msg_json["fields"]["EnergyMeter.triggered_voltage"] = obj.triggered_voltage;
                    msg_json["fields"]["EnergyMeter.triggered_current"] = obj.triggered_current;
                    msg_json["fields"]["EnergyMeter.voltage"]           = obj.voltage;
                    msg_json["fields"]["EnergyMeter.current"]           = obj.current;
                }
            }
            break;

            case 113: // DVStates
            {
                vcu_DVStates_1_0 obj;
                vcu_DVStates_1_0_initialize_(&obj);
                deserialization_result = vcu_DVStates_1_0_deserialize_( &obj, static_cast<const uint8_t*>(transfer.payload), &payload_size);
                if (deserialization_result >= 0)
                {
                    msg_json["measure"] = "vcu";
                    msg_json["fields"]["DVStates.as_state"]            = obj.as_state;
                    msg_json["fields"]["DVStates.ebs_state"]           = obj.ebs_state;
                    msg_json["fields"]["DVStates.ami_state"]           = obj.ami_state;
                    msg_json["fields"]["DVStates.steering_state"]      = obj.steering_state;
                    msg_json["fields"]["DVStates.service_brake_state"] = obj.service_brake_state;
                }
            }
            break;

            case 114: // InsEstimates1
            {
                vcu_InsEstimates1_1_0 obj;
                vcu_InsEstimates1_1_0_initialize_(&obj);
                deserialization_result = vcu_InsEstimates1_1_0_deserialize_( &obj, static_cast<const uint8_t*>(transfer.payload), &payload_size);
                if (deserialization_result >= 0)
                {
                    msg_json["measure"] = "vcu";
                    msg_json["fields"]["InsEstimates1.gps_time_msb"] = obj.gps_time_msb;
                    msg_json["fields"]["InsEstimates1.gps_time_lsb"] = obj.gps_time_lsb;
                    msg_json["fields"]["InsEstimates1.pps_time_msb"] = obj.pps_time_msb;
                    msg_json["fields"]["InsEstimates1.pps_time_lsb"] = obj.pps_time_lsb;
                    msg_json["fields"]["InsEstimates1.lat"]          = obj.lat;
                    msg_json["fields"]["InsEstimates1.lon"]          = obj.lon;
                    msg_json["fields"]["InsEstimates1.alt"]          = obj.alt;
                    msg_json["fields"]["InsEstimates1.pos_std"]      = obj.pos_std;
                    msg_json["fields"]["InsEstimates1.roll"]         = obj.roll;
                    msg_json["fields"]["InsEstimates1.pitch"]        = obj.pitch;
                    msg_json["fields"]["InsEstimates1.yaw"]          = obj.yaw;
                }
            }
            break;

            default: {
                //std::cerr << "Unknown port_id: " << port_id << std::endl;
                return false;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception during deserialization for port " << port_id << ": " << e.what() << std::endl;
        return false;
    }

    if (deserialization_result < 0) {
        std::cerr << "Deserialization failed for port " << port_id << " with error code = " << int(deserialization_result) << std::endl;
        return false;
    }

    return true;
}

// SERIALIZE ==================================================================================================================================

std::vector<uint8_t> Converter::jsonToUdp(const json& j) {

    if (!j.contains("messages")) {
        std::cerr << "Error: JSON object does not contain 'messages'\n";
        return {};
    }

    if (!j["messages"].is_array()) {
        std::cerr << "Error: JSON['messages'] is not an array\n";
        return {};
    }

    std::vector<uint8_t> udp_msg;
    // some can messages can be slightly larger than 64 but most is less than 64 bytes
    // see Converter::CanMessage
    udp_msg.reserve(j["messages"].size() * 64); 

    for (const auto& msg : j["messages"]) {
        std::vector<uint8_t> can_msg = jsonToCan(msg);
        if (can_msg.empty()) {
            std::cerr << "Warning: Failed to serialize a CAN message. Skipping.\n";
            continue;
        }
        udp_msg.insert(udp_msg.end(), can_msg.begin(), can_msg.end());
    }
    return udp_msg;
}
std::vector<uint8_t> Converter::jsonToCan(const json& msg_json)
{
    CanardPortID port_id = 0;
    uint64_t timestamp = 0;
    std::vector<uint8_t> buffer(256, 0U);
    size_t inout_buffer_size = buffer.size();

    // first serializing json into a payload with revolve dsdl definitions
    {
        // Check the presence of mandatory fields
        if (!msg_json.contains("port_id")) {
            std::cerr << "JSON error: Missing 'port_id'\n";
            return {};
        }
        if (!msg_json.contains("timestamp")) {
            std::cerr << "JSON error: Missing 'timestamp'\n";
            return {};
        }
        if (!msg_json.contains("fields")){
            std::cerr << "JSON error: Missing 'fields'\n";
            return {};
        }
        try {
            port_id   = msg_json["port_id"].get<CanardPortID>();
            timestamp = msg_json["timestamp"].get<uint64_t>();
        }
        catch (const std::exception& e) {
            std::cerr << "JSON type error reading 'port_id' or 'timestamp': " << e.what() << "\n";
            return {};
        }
        
        int8_t serialization_result = -1;
        try
        {
            switch (port_id)
            {
                case 8:  // EbsActivation
                {
                    vcu_EbsActivation_Request_1_0 obj;
                    vcu_EbsActivation_Request_1_0_initialize_(&obj);
                    obj.activate_ebs = msg_json["fields"]["EbsActivation.activate_ebs"].get<uint8_t>();
                    serialization_result = vcu_EbsActivation_Request_1_0_serialize_( &obj, buffer.data(), &inout_buffer_size);
                    break;
                }
                case 10: // DriveMode
                {
                    vcu_DriveMode_Request_1_0 obj;
                    vcu_DriveMode_Request_1_0_initialize_(&obj);
                    obj.command = static_cast<uint8_t>(msg_json["fields"]["DriveMode.command"]);
                    serialization_result = vcu_DriveMode_Request_1_0_serialize_(&obj, buffer.data(), &inout_buffer_size);
                    break;
                }
                case 13: // Config
                {
                    vcu_Config_Request_1_0 obj;
                    vcu_Config_Request_1_0_initialize_(&obj);
                    obj.st_trq       = msg_json["fields"]["Config.st_trq"].get<float>();
                    obj.st_rpm       = msg_json["fields"]["Config.st_rpm"].get<float>();
                    obj.pitch_offset = msg_json["fields"]["Config.pitch_offset"].get<float>();
                    obj.roll_offset  = msg_json["fields"]["Config.roll_offset"].get<float>();
                    serialization_result = vcu_Config_Request_1_0_serialize_(&obj,buffer.data(),&inout_buffer_size);
                    break;
                }
                case 102: // INS
                {
                    vcu_INS_1_0 obj;
                    vcu_INS_1_0_initialize_(&obj);
                    obj.vx              = msg_json["fields"]["INS.vx"].get<float>();
                    obj.vy              = msg_json["fields"]["INS.vy"].get<float>();
                    obj.vz              = msg_json["fields"]["INS.vz"].get<float>();
                    obj.ax              = msg_json["fields"]["INS.ax"].get<float>();
                    obj.ay              = msg_json["fields"]["INS.ay"].get<float>();
                    obj.az              = msg_json["fields"]["INS.az"].get<float>();
                    obj.roll            = msg_json["fields"]["INS.roll"].get<float>();
                    obj.pitch           = msg_json["fields"]["INS.pitch"].get<float>();
                    obj.yaw             = msg_json["fields"]["INS.yaw"].get<float>();
                    obj.roll_rate       = msg_json["fields"]["INS.roll_rate"].get<float>();
                    obj.pitch_rate      = msg_json["fields"]["INS.pitch_rate"].get<float>();
                    obj.yaw_rate        = msg_json["fields"]["INS.yaw_rate"].get<float>();
                    obj.roll_rate_dt    = msg_json["fields"]["INS.roll_rate_dt"].get<float>();
                    obj.pitch_rate_dt   = msg_json["fields"]["INS.pitch_rate_dt"].get<float>();
                    obj.yaw_rate_dt     = msg_json["fields"]["INS.yaw_rate_dt"].get<float>();
                    serialization_result = vcu_INS_1_0_serialize_( &obj, buffer.data(), &inout_buffer_size);
                    break;
                }
                case 111: // EnergyMeter
                {
                    vcu_EnergyMeter_1_0 obj;
                    vcu_EnergyMeter_1_0_initialize_(&obj);
                    obj.counter             = msg_json["fields"]["EnergyMeter.counter"].get<uint32_t>();
                    obj.ready               = msg_json["fields"]["EnergyMeter.ready"].get<bool>();
                    obj.logging             = msg_json["fields"]["EnergyMeter.logging"].get<bool>();
                    obj.triggered_voltage   = msg_json["fields"]["EnergyMeter.triggered_voltage"].get<bool>();
                    obj.triggered_current   = msg_json["fields"]["EnergyMeter.triggered_current"].get<bool>();
                    obj.voltage             = msg_json["fields"]["EnergyMeter.voltage"].get<float>();
                    obj.current             = msg_json["fields"]["EnergyMeter.current"].get<float>();
                    serialization_result = vcu_EnergyMeter_1_0_serialize_(&obj, buffer.data(), &inout_buffer_size);
                    break;
                }
                case 113: // DVStates
                {
                    vcu_DVStates_1_0 obj;
                    vcu_DVStates_1_0_initialize_(&obj);
                    obj.as_state            = msg_json["fields"]["DVStates.as_state"].get<uint8_t>();
                    obj.ebs_state           = msg_json["fields"]["DVStates.ebs_state"].get<uint8_t>();
                    obj.ami_state           = msg_json["fields"]["DVStates.ami_state"].get<uint8_t>();
                    obj.steering_state      = msg_json["fields"]["DVStates.steering_state"].get<uint8_t>();
                    obj.service_brake_state = msg_json["fields"]["DVStates.service_brake_state"].get<uint8_t>();
                    serialization_result = vcu_DVStates_1_0_serialize_(&obj,buffer.data(),&inout_buffer_size);
                    break;
                }
                case 114: // InsEstimates1
                {
                    vcu_InsEstimates1_1_0 obj;
                    vcu_InsEstimates1_1_0_initialize_(&obj);
                    obj.gps_time_msb    = msg_json["fields"]["InsEstimates1.gps_time_msb"].get<uint32_t>();
                    obj.gps_time_lsb    = msg_json["fields"]["InsEstimates1.gps_time_lsb"].get<uint32_t>();
                    obj.pps_time_msb    = msg_json["fields"]["InsEstimates1.pps_time_msb"].get<uint32_t>();
                    obj.pps_time_lsb    = msg_json["fields"]["InsEstimates1.pps_time_lsb"].get<uint32_t>();
                    obj.lat             = msg_json["fields"]["InsEstimates1.lat"].get<double>();
                    obj.lon             = msg_json["fields"]["InsEstimates1.lon"].get<double>();
                    obj.alt             = msg_json["fields"]["InsEstimates1.alt"].get<float>();
                    obj.pos_std         = msg_json["fields"]["InsEstimates1.pos_std"].get<float>();
                    obj.roll            = msg_json["fields"]["InsEstimates1.roll"].get<float>();
                    obj.pitch           = msg_json["fields"]["InsEstimates1.pitch"].get<float>();
                    obj.yaw             = msg_json["fields"]["InsEstimates1.yaw"].get<float>();
                    serialization_result = vcu_InsEstimates1_1_0_serialize_(&obj,buffer.data(),&inout_buffer_size);
                    break;
                }

                default:
                {
                    std::cerr << "Error: Unknown or unsupported port_id " << port_id << std::endl;
                    return {};
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Exception filling UAVCAN object from JSON for port_id " << port_id << ": " << e.what() << std::endl;
            return {};
        }

        if (serialization_result < 0) {
            std::cerr << "Serialization failed (result=" << int(serialization_result) << ").\n";
            return {};
        }
    }

    // We now have the serialized payload in [0..inout_buffer_size-1].
    // Then we construct the can message which is ready to be sticked into a udp message
    {
        const size_t capacity   = 1;   // We only want a single frame
        const size_t mtu_bytes  = 64;  // CAN FD max (64 bytes of payload)
        static uint8_t my_message_transfer_id = 0;

        CanardTransferMetadata metadata;
        std::memset(&metadata, 0, sizeof(metadata));
        metadata.priority       = CanardPriorityNominal;      // 4
        metadata.transfer_kind  = CanardTransferKindMessage;  // 0
        metadata.port_id        = port_id;                    // subject-ID = port_id
        metadata.remote_node_id = CANARD_NODE_ID_UNSET;       // broadcast
        metadata.transfer_id    = my_message_transfer_id++;

        // Initialize a fresh TX queue
        CanardTxQueue tx_queue = canardTxInit(capacity, mtu_bytes);

        // The push
        int32_t push_result = canardTxPush(&tx_queue, &m_canard_instance, 0, &metadata, inout_buffer_size, buffer.data());
        if (push_result < 0) {
            std::cerr << "Error: canardTxPush() failed with code " << push_result << std::endl;
            return {};
        }

        // Peek at the queue
        const CanardTxQueueItem* tx_item = canardTxPeek(&tx_queue);
        if (!tx_item) {
            std::cerr << "Error: canardTxPeek() provided no data.\n";
            return {};
        }

        // The actual frame with ID + payload
        const CanardFrame& frame = tx_item->frame;
        const uint8_t payload_size = static_cast<uint8_t>(frame.payload_size);

        // ----------------------------------------
        // Build our output vector:
        //   8 bytes of timestamp
        // + 4 bytes of CAN ID
        // + 1 byte  of payload_size
        // + payload_size bytes
        // ----------------------------------------
        std::vector<uint8_t> can;
        can.reserve(8 + 4 + 1 + payload_size);

        // 1) Insert 8-byte timestamp (big-endian or little-endian as needed).
        const uint64_t be_timestamp = toBigEndian64(timestamp);
        const auto* ts_ptr = reinterpret_cast<const uint8_t*>(&be_timestamp);
        can.insert(can.end(), ts_ptr, ts_ptr + 8);

        // 2) Insert 4-byte CAN ID (some 29-bit masked ID).
        const uint32_t be_can_id = toBigEndian32(frame.extended_can_id & 0x1FFFFFFF);
        const auto* can_id_ptr = reinterpret_cast<const uint8_t*>(&be_can_id);
        can.insert(can.end(), can_id_ptr, can_id_ptr + 4);

        // 3) Insert 1-byte payload size
        can.push_back(payload_size);

        // 4) Insert the actual payload
        const uint8_t* payload_ptr = static_cast<const uint8_t*>(frame.payload);
        can.insert(can.end(), payload_ptr, payload_ptr + payload_size);

        // Pop the item off the queue
        canardTxPop(&tx_queue, tx_item);

        // Ensure no leftover multi-frame data
        if (canardTxPeek(&tx_queue)) {
            std::cerr << "Error: The given payload is too large (multi-frame not supported here)\n";
            return {};
        }
        return can;
    }
}