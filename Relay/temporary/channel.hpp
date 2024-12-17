#pragma once 

#include <vector>
#include <mutex>
#include <string>
#include <unordered_map>
#include <variant>
#include <cstdint>

using ChannelValue_t = std::variant<double, float, uint32_t, uint16_t, uint8_t, bool>;

class Channel {
private:
    std::vector<double> m_time;
    std::vector<ChannelValue_t> m_value;
    std::vector<bool> m_exists_in_local_influxdb;
    std::vector<bool> m_exists_in_global_influxdb;
    mutable std::mutex m_mutex;
    bool m_is_sorted;
    bool m_updated;

    std::string m_log_id; // CAN_YYYY_MM_dd(HHmmss)
    std::unordered_map<std::string, std::string> m_tags; // must contain keys measure, field and can contain car, competition, driver, event, lap, log_type

    static const std::vector<std::string> m_measures;
    static const std::vector<std::string> m_field_ams;
    static const std::vector<std::string> m_field_ccc;
    static const std::vector<std::string> m_field_common;
    static const std::vector<std::string> m_field_dashboard;
    static const std::vector<std::string> m_field_dcu;
    static const std::vector<std::string> m_field_inverter;
    static const std::vector<std::string> m_field_pu;
    static const std::vector<std::string> m_field_sensors;
    static const std::vector<std::string> m_field_tv;
    static const std::vector<std::string> m_field_vcu;
    static const std::vector<std::string> m_cars;
    static const std::vector<std::string> m_competitions;
    static const std::vector<std::string> m_drivers; // Corrected spelling
    static const std::vector<std::string> m_events;
    static const std::vector<std::string> m_laps;
    static const std::vector<std::string> m_log_types;

public:
    Channel(const std::string& log_id, const std::unordered_map<std::string, std::string>& tags);
    Channel(const Channel& other);
    Channel(Channel&&) = delete;
    ~Channel();

    Channel&                                        operator=(const Channel&) = delete;
    Channel&                                        operator=(Channel&&) = delete;

    void                                            addDatapoint(double time, const ChannelValue_t& value);
    void                                            addDatapoints(const std::vector<double>& times, const std::vector<ChannelValue_t>& values);
    void                                            addChannel(const Channel& other);

    void                                            setLogIdForNow();
    long long                                       getTimeForLogId() const;
    std::string                                     getLogId() const;
    std::string                                     getMeasurement() const;
    std::string                                     getField() const;
    std::unordered_map<std::string, std::string>    getTags() const;
    ChannelValue_t                                  getValue(double time) const;
    std::vector<ChannelValue_t>                     getValues() const;
    std::vector<double>                             getTimes() const;

    bool                                            isDataSorted() const;

    void                                            sortData();
};
