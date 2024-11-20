#pragma once

#include <functional>
#include <unordered_map>
#include <iostream>
#include "libcanard/canard.h"
#include "DataManager.hpp"

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


using DeserializationMap = std::unordered_map<CanardPortID, std::function<void(DataManager&, const CanardRxTransfer&)>>;

DeserializationMap createDeserializationMap() 
{
    DeserializationMap deserialization_map;

    deserialization_map[8] = [](DataManager& data_manager, const CanardRxTransfer& transfer) {
        vcu_EbsActivation_Request_1_0 data;
        size_t inout_buffer_size_bytes = transfer.payload_size;
        int8_t deserialization_result = vcu_EbsActivation_Request_1_0_deserialize_(&data, (uint8_t*)transfer.payload, &inout_buffer_size_bytes);
        if (deserialization_result < 0) {
            std::cerr << "Deserialization failed: " << static_cast<int>(deserialization_result) << std::endl;
        } else {
            data_manager.AddDatapoint<uint8_t>("CAN_2024-11-20(142000)", "vcu", "EbsActivation.activate_ebs", transfer.timestamp_usec, data.activate_ebs);
        }
    };

    deserialization_map[10] = [](DataManager& data_manager, const CanardRxTransfer& transfer) {
        vcu_DriveMode_Request_1_0 data;
        size_t inout_buffer_size_bytes = transfer.payload_size;
        int8_t deserialization_result = vcu_DriveMode_Request_1_0_deserialize_(&data, (uint8_t*)transfer.payload, &inout_buffer_size_bytes);
        if (deserialization_result < 0) {
            std::cerr << "Deserialization failed: " << static_cast<int>(deserialization_result) << std::endl;
        } else {
            data_manager.AddDatapoint<uint8_t>("CAN_2024-11-20(142000)", "vcu", "DriveMode.command", transfer.timestamp_usec, data.command);
        }
    };

    deserialization_map[13] = [](DataManager& data_manager, const CanardRxTransfer& transfer) {
        vcu_Config_Request_1_0 data;
        size_t inout_buffer_size_bytes = transfer.payload_size;
        int8_t deserialization_result = vcu_Config_Request_1_0_deserialize_(&data, (uint8_t*)transfer.payload, &inout_buffer_size_bytes);
        if (deserialization_result < 0) {
            std::cerr << "Deserialization failed: " << static_cast<int>(deserialization_result) << std::endl;
        } else {
            data_manager.AddDatapoint<float>("CAN_2024-11-20(142000)", "vcu", "Config.st_trq", transfer.timestamp_usec, data.st_trq);
            data_manager.AddDatapoint<float>("CAN_2024-11-20(142000)", "vcu", "Config.st_rpm", transfer.timestamp_usec, data.st_rpm);
            data_manager.AddDatapoint<float>("CAN_2024-11-20(142000)", "vcu", "Config.pitch_offset", transfer.timestamp_usec, data.pitch_offset);
            data_manager.AddDatapoint<float>("CAN_2024-11-20(142000)", "vcu", "Config.roll_offset", transfer.timestamp_usec, data.roll_offset);
        }
    };

    deserialization_map[102] = [](DataManager& data_manager, const CanardRxTransfer& transfer) {
        vcu_INS_1_0 data;
        size_t inout_buffer_size_bytes = transfer.payload_size;
        int8_t deserialization_result = vcu_INS_1_0_deserialize_(&data, (uint8_t*)transfer.payload, &inout_buffer_size_bytes);
        if (deserialization_result < 0) {
            std::cerr << "Deserialization failed: " << static_cast<int>(deserialization_result) << std::endl;
        } else {
            data_manager.AddDatapoint<float>("CAN_2024-11-20(142000)", "vcu", "INS.vx", transfer.timestamp_usec, data.vx);
            data_manager.AddDatapoint<float>("CAN_2024-11-20(142000)", "vcu", "INS.vy", transfer.timestamp_usec, data.vy);
            data_manager.AddDatapoint<float>("CAN_2024-11-20(142000)", "vcu", "INS.vz", transfer.timestamp_usec, data.vz);
            data_manager.AddDatapoint<float>("CAN_2024-11-20(142000)", "vcu", "INS.ax", transfer.timestamp_usec, data.ax);
            data_manager.AddDatapoint<float>("CAN_2024-11-20(142000)", "vcu", "INS.ay", transfer.timestamp_usec, data.ay);
            data_manager.AddDatapoint<float>("CAN_2024-11-20(142000)", "vcu", "INS.az", transfer.timestamp_usec, data.az);
            data_manager.AddDatapoint<float>("CAN_2024-11-20(142000)", "vcu", "INS.roll", transfer.timestamp_usec, data.roll);
            data_manager.AddDatapoint<float>("CAN_2024-11-20(142000)", "vcu", "INS.pitch", transfer.timestamp_usec, data.pitch);
            data_manager.AddDatapoint<float>("CAN_2024-11-20(142000)", "vcu", "INS.yaw", transfer.timestamp_usec, data.yaw);
            data_manager.AddDatapoint<float>("CAN_2024-11-20(142000)", "vcu", "INS.roll_rate", transfer.timestamp_usec, data.roll_rate);
            data_manager.AddDatapoint<float>("CAN_2024-11-20(142000)", "vcu", "INS.pitch_rate", transfer.timestamp_usec, data.pitch_rate);
            data_manager.AddDatapoint<float>("CAN_2024-11-20(142000)", "vcu", "INS.yaw_rate", transfer.timestamp_usec, data.yaw_rate);
            data_manager.AddDatapoint<float>("CAN_2024-11-20(142000)", "vcu", "INS.roll_rate_dt", transfer.timestamp_usec, data.roll_rate_dt);
            data_manager.AddDatapoint<float>("CAN_2024-11-20(142000)", "vcu", "INS.pitch_rate_dt", transfer.timestamp_usec, data.pitch_rate_dt);
            data_manager.AddDatapoint<float>("CAN_2024-11-20(142000)", "vcu", "INS.yaw_rate_dt", transfer.timestamp_usec, data.yaw_rate_dt);
        }
    };

    deserialization_map[111] = [](DataManager& data_manager, const CanardRxTransfer& transfer) {
        vcu_EnergyMeter_1_0 data;
        size_t inout_buffer_size_bytes = transfer.payload_size;
        int8_t deserialization_result = vcu_EnergyMeter_1_0_deserialize_(&data, (uint8_t*)transfer.payload, &inout_buffer_size_bytes);
        if (deserialization_result < 0) {
            std::cerr << "Deserialization failed: " << static_cast<int>(deserialization_result) << std::endl;
        } else {
            data_manager.AddDatapoint<uint8_t>("CAN_2024-11-20(142000)", "vcu", "EnergyMeter.counter", transfer.timestamp_usec, data.counter);
            data_manager.AddDatapoint<bool>("CAN_2024-11-20(142000)", "vcu", "EnergyMeter.ready", transfer.timestamp_usec, data.ready);
            data_manager.AddDatapoint<bool>("CAN_2024-11-20(142000)", "vcu", "EnergyMeter.logging", transfer.timestamp_usec, data.logging);
            data_manager.AddDatapoint<bool>("CAN_2024-11-20(142000)", "vcu", "EnergyMeter.triggered_voltage", transfer.timestamp_usec, data.triggered_voltage);
            data_manager.AddDatapoint<bool>("CAN_2024-11-20(142000)", "vcu", "EnergyMeter.triggered_current", transfer.timestamp_usec, data.triggered_current);
            data_manager.AddDatapoint<float>("CAN_2024-11-20(142000)", "vcu", "EnergyMeter.voltage", transfer.timestamp_usec, data.voltage);
            data_manager.AddDatapoint<float>("CAN_2024-11-20(142000)", "vcu", "EnergyMeter.current", transfer.timestamp_usec, data.current);
        }
    };

    deserialization_map[113] = [](DataManager& data_manager, const CanardRxTransfer& transfer) {
        vcu_DVStates_1_0 data;
        size_t inout_buffer_size_bytes = transfer.payload_size;
        int8_t deserialization_result = vcu_DVStates_1_0_deserialize_(&data, (uint8_t*)transfer.payload, &inout_buffer_size_bytes);
        if (deserialization_result < 0) {
            std::cerr << "Deserialization failed: " << static_cast<int>(deserialization_result) << std::endl;
        } else {
            data_manager.AddDatapoint<uint8_t>("CAN_2024-11-20(142000)", "vcu", "DVStates.as_state", transfer.timestamp_usec, data.as_state);
            data_manager.AddDatapoint<uint8_t>("CAN_2024-11-20(142000)", "vcu", "DVStates.ebs_state", transfer.timestamp_usec, data.ebs_state);
            data_manager.AddDatapoint<uint8_t>("CAN_2024-11-20(142000)", "vcu", "DVStates.ami_state", transfer.timestamp_usec, data.ami_state);
            data_manager.AddDatapoint<bool>("CAN_2024-11-20(142000)", "vcu", "DVStates.steering_state", transfer.timestamp_usec, data.steering_state);
            data_manager.AddDatapoint<uint8_t>("CAN_2024-11-20(142000)", "vcu", "DVStates.service_brake_state", transfer.timestamp_usec, data.service_brake_state);
        }
    };

    deserialization_map[114] = [](DataManager& data_manager, const CanardRxTransfer& transfer) {
        vcu_InsEstimates1_1_0 data;
        size_t inout_buffer_size_bytes = transfer.payload_size;
        int8_t deserialization_result = vcu_InsEstimates1_1_0_deserialize_(&data, (uint8_t*)transfer.payload, &inout_buffer_size_bytes);
        if (deserialization_result < 0) {
            std::cerr << "Deserialization failed: " << static_cast<int>(deserialization_result) << std::endl;
        } else {
            data_manager.AddDatapoint<uint32_t>("CAN_2024-11-20(142000)", "vcu", "InsEstimates1.gps_time_msb", transfer.timestamp_usec, data.gps_time_msb);
            data_manager.AddDatapoint<uint32_t>("CAN_2024-11-20(142000)", "vcu", "InsEstimates1.gps_time_lsb", transfer.timestamp_usec, data.gps_time_lsb);
            data_manager.AddDatapoint<uint32_t>("CAN_2024-11-20(142000)", "vcu", "InsEstimates1.pps_time_msb", transfer.timestamp_usec, data.pps_time_msb);
            data_manager.AddDatapoint<uint32_t>("CAN_2024-11-20(142000)", "vcu", "InsEstimates1.pps_time_lsb", transfer.timestamp_usec, data.pps_time_lsb);
            data_manager.AddDatapoint<double>("CAN_2024-11-20(142000)", "vcu", "InsEstimates1.lat", transfer.timestamp_usec, data.lat);
            data_manager.AddDatapoint<double>("CAN_2024-11-20(142000)", "vcu", "InsEstimates1.lon", transfer.timestamp_usec, data.lon);
            data_manager.AddDatapoint<double>("CAN_2024-11-20(142000)", "vcu", "InsEstimates1.alt", transfer.timestamp_usec, data.alt);
            data_manager.AddDatapoint<float>("CAN_2024-11-20(142000)", "vcu", "InsEstimates1.pos_std", transfer.timestamp_usec, data.pos_std);
            data_manager.AddDatapoint<float>("CAN_2024-11-20(142000)", "vcu", "InsEstimates1.roll", transfer.timestamp_usec, data.roll);
            data_manager.AddDatapoint<float>("CAN_2024-11-20(142000)", "vcu", "InsEstimates1.pitch", transfer.timestamp_usec, data.pitch);
            data_manager.AddDatapoint<float>("CAN_2024-11-20(142000)", "vcu", "InsEstimates1.yaw", transfer.timestamp_usec, data.yaw);
        }
    };

    return deserialization_map;
}
