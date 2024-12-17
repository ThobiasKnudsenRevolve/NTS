#include "udp_can_converter.hpp"
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

bool UdpCanConverter::deserializeToJson(CanardPortID portId, const CanardRxTransfer& transfer, json& msg_json) {
    msg_json["port_id"] = portId;
    json fields_json = json::object();

    size_t inout_buffer_size_bytes = transfer.payload_size;
    int8_t deserialization_result = -1;

    switch (portId) {
        case 8: {
            msg_json["measure"] = "vcu";
            msg_json["timestamp"] = transfer.timestamp_usec;

            vcu_EbsActivation_Request_1_0 data;
            deserialization_result = vcu_EbsActivation_Request_1_0_deserialize_(&data, 
                            (uint8_t*)transfer.payload, &inout_buffer_size_bytes);
            if (deserialization_result >= 0) {
                fields_json["EbsActivation.activate_ebs"] = data.activate_ebs;
            }
        } break;

        case 10: {
            msg_json["measure"] = "vcu";
            msg_json["timestamp"] = transfer.timestamp_usec;

            vcu_DriveMode_Request_1_0 data;
            deserialization_result = vcu_DriveMode_Request_1_0_deserialize_(&data, 
                            (uint8_t*)transfer.payload, &inout_buffer_size_bytes);
            if (deserialization_result >= 0) {
                fields_json["DriveMode.command"] = data.command;
            }
        } break;

        case 13: {
            msg_json["measure"] = "vcu";
            msg_json["timestamp"] = transfer.timestamp_usec;

            vcu_Config_Request_1_0 data;
            deserialization_result = vcu_Config_Request_1_0_deserialize_(&data, 
                            (uint8_t*)transfer.payload, &inout_buffer_size_bytes);
            if (deserialization_result >= 0) {
                fields_json["Config.st_trq"] = data.st_trq;
                fields_json["Config.st_rpm"] = data.st_rpm;
                fields_json["Config.pitch_offset"] = data.pitch_offset;
                fields_json["Config.roll_offset"] = data.roll_offset;
            }
        } break;

        case 102: {
            msg_json["measure"] = "vcu";
            msg_json["timestamp"] = transfer.timestamp_usec;

            vcu_INS_1_0 data;
            deserialization_result = vcu_INS_1_0_deserialize_(&data, 
                            (uint8_t*)transfer.payload, &inout_buffer_size_bytes);
            if (deserialization_result >= 0) {
                fields_json["INS.vx"] = data.vx;
                fields_json["INS.vy"] = data.vy;
                fields_json["INS.vz"] = data.vz;
                fields_json["INS.ax"] = data.ax;
                fields_json["INS.ay"] = data.ay;
                fields_json["INS.az"] = data.az;
                fields_json["INS.roll"] = data.roll;
                fields_json["INS.pitch"] = data.pitch;
                fields_json["INS.yaw"] = data.yaw;
                fields_json["INS.roll_rate"] = data.roll_rate;
                fields_json["INS.pitch_rate"] = data.pitch_rate;
                fields_json["INS.yaw_rate"] = data.yaw_rate;
                fields_json["INS.roll_rate_dt"] = data.roll_rate_dt;
                fields_json["INS.pitch_rate_dt"] = data.pitch_rate_dt;
                fields_json["INS.yaw_rate_dt"] = data.yaw_rate_dt;
            }
        } break;

        case 111: {
            msg_json["measure"] = "vcu";
            msg_json["timestamp"] = transfer.timestamp_usec;

            vcu_EnergyMeter_1_0 data;
            deserialization_result = vcu_EnergyMeter_1_0_deserialize_(&data, 
                            (uint8_t*)transfer.payload, &inout_buffer_size_bytes);
            if (deserialization_result >= 0) {
                fields_json["EnergyMeter.counter"] = data.counter;
                fields_json["EnergyMeter.ready"] = data.ready;
                fields_json["EnergyMeter.logging"] = data.logging;
                fields_json["EnergyMeter.triggered_voltage"] = data.triggered_voltage;
                fields_json["EnergyMeter.triggered_current"] = data.triggered_current;
                fields_json["EnergyMeter.voltage"] = data.voltage;
                fields_json["EnergyMeter.current"] = data.current;
            }
        } break;

        case 113: {
            msg_json["measure"] = "vcu";
            msg_json["timestamp"] = transfer.timestamp_usec;

            vcu_DVStates_1_0 data;
            deserialization_result = vcu_DVStates_1_0_deserialize_(&data, 
                            (uint8_t*)transfer.payload, &inout_buffer_size_bytes);
            if (deserialization_result >= 0) {
                fields_json["DVStates.as_state"] = data.as_state;
                fields_json["DVStates.ebs_state"] = data.ebs_state;
                fields_json["DVStates.ami_state"] = data.ami_state;
                fields_json["DVStates.steering_state"] = data.steering_state;
                fields_json["DVStates.service_brake_state"] = data.service_brake_state;
            }
        } break;

        case 114: {
            msg_json["measure"] = "vcu";
            msg_json["timestamp"] = transfer.timestamp_usec;

            vcu_InsEstimates1_1_0 data;
            deserialization_result = vcu_InsEstimates1_1_0_deserialize_(&data, 
                            (uint8_t*)transfer.payload, &inout_buffer_size_bytes);
            if (deserialization_result >= 0) {
                fields_json["InsEstimates1.gps_time_msb"] = data.gps_time_msb;
                fields_json["InsEstimates1.gps_time_lsb"] = data.gps_time_lsb;
                fields_json["InsEstimates1.pps_time_msb"] = data.pps_time_msb;
                fields_json["InsEstimates1.pps_time_lsb"] = data.pps_time_lsb;
                fields_json["InsEstimates1.lat"] = data.lat;
                fields_json["InsEstimates1.lon"] = data.lon;
                fields_json["InsEstimates1.alt"] = data.alt;
                fields_json["InsEstimates1.pos_std"] = data.pos_std;
                fields_json["InsEstimates1.roll"] = data.roll;
                fields_json["InsEstimates1.pitch"] = data.pitch;
                fields_json["InsEstimates1.yaw"] = data.yaw;
            }
        } break;

        default:
            // Unknown port, ignore
            deserialization_result = -1;
            break;
    }

    if (deserialization_result < 0) {
        //std::cerr << "Deserialization failed for port " << portId << ": " << static_cast<int>(deserialization_result) << "\n";
        return false;
    }

    msg_json["fields"] = fields_json;
    return true;
}