// channel.cpp
#include "channel.hpp"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

// Initialize static member variables
const std::vector<std::string> Channel::m_measures = {"ams", "ccc", "common", "dashboard", "dcu", "inverter", "pu", "sensors", "tv", "vcu"};
const std::vector<std::string> Channel::m_field_ams = {"field1", "field2"}; // Example fields
const std::vector<std::string> Channel::m_field_ccc = {"field1", "field2"};
const std::vector<std::string> Channel::m_field_common = {"field1", "field2"};
const std::vector<std::string> Channel::m_field_dashboard = {"field1", "field2"};
const std::vector<std::string> Channel::m_field_dcu = {"field1", "field2"};
const std::vector<std::string> Channel::m_field_inverter = {"field1", "field2"};
const std::vector<std::string> Channel::m_field_pu = {"field1", "field2"};
const std::vector<std::string> Channel::m_field_sensors = {"field1", "field2"};
const std::vector<std::string> Channel::m_field_tv = {"field1", "field2"};
const std::vector<std::string> Channel::m_field_vcu = {"field1", "field2"};
const std::vector<std::string> Channel::m_cars = {"Hera", "Lyra"};
const std::vector<std::string> Channel::m_competitions = {"FSEast", "FSG"};
const std::vector<std::string> Channel::m_drivers = {"Driverless", "Stian Persson Lie", "Stian Lie", "Håvard V. Hagerup", "Håvard Hagerup", "Mads Engesvoll", "Magnus Husby", "Eivind Due-Tønnesen", "Other", "Not Relevant"};
const std::vector<std::string> Channel::m_events = {"Autocross", "Endurance", "Training", "Acceleration", "Skidpad", "Brake Test", "BrakeTest", "Shakedown", "Trackdrive", "Other"};
const std::vector<std::string> Channel::m_laps = {"ToGarage", "ToStart", "InPits", "CustomLap"};
const std::vector<std::string> Channel::m_log_types = {"TEST_DRIVE", "COMPETITION_DRIVE"};

// Constructor
Channel::Channel(const std::string& log_id, const std::unordered_map<std::string, std::string>& tags)
    : m_log_id(log_id), m_tags(tags), m_is_sorted(false), m_updated(false) {
    // Initialization logic if necessary
}

// Copy Constructor
Channel::Channel(const Channel& other) {
    std::lock_guard<std::mutex> lock(other.m_mutex);
    m_time = other.m_time;
    m_value = other.m_value;
    m_exists_in_local_influxdb = other.m_exists_in_local_influxdb;
    m_exists_in_global_influxdb = other.m_exists_in_global_influxdb;
    m_is_sorted = other.m_is_sorted;
    m_updated = other.m_updated;
    m_log_id = other.m_log_id;
    m_tags = other.m_tags;
    // Static members are shared and already initialized
}

// Destructor
Channel::~Channel() {
    // Cleanup if necessary
}

// Add a single datapoint with thread safety
void Channel::addDatapoint(double time, const ChannelValue_t& value) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_time.empty() && time <= m_time.back()) {
        m_is_sorted = false;
    }

    m_time.emplace_back(time);
    m_value.emplace_back(value);
    m_updated = true;
}

// Add multiple datapoints with thread safety
void Channel::addDatapoints(const std::vector<double>& times, const std::vector<ChannelValue_t>& values) {
    if (times.size() != values.size()) {
        throw std::invalid_argument("times.size() != values.size()");
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    m_time.reserve(m_time.size() + times.size());
    m_value.reserve(m_value.size() + values.size());

    m_time.insert(m_time.end(), times.begin(), times.end());
    m_value.insert(m_value.end(), values.begin(), values.end());

    m_is_sorted = false;
    m_updated = true;
}

// Add another channel's data into this channel with thread safety
void Channel::addChannel(const Channel& other) {
    std::scoped_lock lock(m_mutex, other.m_mutex);

    m_time.insert(m_time.end(), other.m_time.begin(), other.m_time.end());
    m_value.insert(m_value.end(), other.m_value.begin(), other.m_value.end());
    m_is_sorted = false;
    m_updated = true;
}

void Channel::setLogIdForNow() {
    // Get current time
    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
#ifdef _WIN32
    localtime_s(&tm_buf, &tt);
#else
    // Using thread-safe localtime_r on POSIX systems
    localtime_r(&tt, &tm_buf);
#endif

    // Format: CAN_YYYY_MM_dd(HHmmss)
    // Example: CAN_2024_12_17(153045)
    std::ostringstream oss;
    oss << "CAN_"
        << (tm_buf.tm_year + 1900) << "_"
        << std::setw(2) << std::setfill('0') << (tm_buf.tm_mon + 1) << "_"
        << std::setw(2) << std::setfill('0') << tm_buf.tm_mday << "("
        << std::setw(2) << std::setfill('0') << tm_buf.tm_hour
        << std::setw(2) << std::setfill('0') << tm_buf.tm_min
        << std::setw(2) << std::setfill('0') << tm_buf.tm_sec
        << ")";

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_log_id = oss.str();
    }
}

long long Channel::getTimeForLogId() const {
    // m_log_id format: "CAN_YYYY_MM_dd(HHmmss)"
    // Example: CAN_2024_12_17(153045)
    // We need to parse this string back into a std::tm.

    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_log_id.size() < sizeof("CAN_YYYY_MM_dd(HHmmss)") - 1) {
        throw std::runtime_error("m_log_id is not in the correct format.");
    }

    // Remove "CAN_" prefix
    const std::string prefix = "CAN_";
    if (m_log_id.compare(0, prefix.size(), prefix) != 0) {
        throw std::runtime_error("m_log_id does not start with 'CAN_'.");
    }

    std::string datetime_str = m_log_id.substr(prefix.size()); 
    // Now datetime_str should be "YYYY_MM_dd(HHmmss)"

    // We can parse manually:
    // Expected layout: YYYY_MM_dd(HHmmss)
    // Positions: YYYY = [0..3], '_' at [4], MM = [5..6], '_' at [7], dd = [8..9], '(' at [10], HHmmss at [11..16], ')' at [17]
    if (datetime_str.size() < 18 || datetime_str[4] != '_' || datetime_str[7] != '_' || datetime_str[10] != '(' || datetime_str.back() != ')') {
        throw std::runtime_error("m_log_id format is incorrect.");
    }

    std::string year_str = datetime_str.substr(0, 4);
    std::string mon_str  = datetime_str.substr(5, 2);
    std::string day_str  = datetime_str.substr(8, 2);
    std::string time_str = datetime_str.substr(11, 6); // HHmmss

    int year = std::stoi(year_str);
    int mon = std::stoi(mon_str);
    int day = std::stoi(day_str);

    if (time_str.size() != 6) {
        throw std::runtime_error("Time portion (HHmmss) is invalid.");
    }

    int hour   = std::stoi(time_str.substr(0, 2));
    int minute = std::stoi(time_str.substr(2, 2));
    int second = std::stoi(time_str.substr(4, 2));

    std::tm tm_buf{};
    tm_buf.tm_year = year - 1900;
    tm_buf.tm_mon  = mon - 1;  // tm_mon is 0-based
    tm_buf.tm_mday = day;
    tm_buf.tm_hour = hour;
    tm_buf.tm_min  = minute;
    tm_buf.tm_sec  = second;
    tm_buf.tm_isdst = -1; // let mktime determine if DST is in effect

    // Convert to time_t
    std::time_t posix_time = std::mktime(&tm_buf);
    if (posix_time == -1) {
        throw std::runtime_error("Could not convert parsed time to time_t.");
    }

    return static_cast<long long>(posix_time);
}

std::string Channel::getLogId() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_log_id;
}

std::string Channel::getMeasurement() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    // Ensure "measure" exists in m_tags
    return m_tags.at("measure");
}

std::string Channel::getField() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    // Ensure "field" exists in m_tags
    return m_tags.at("field");
}

std::unordered_map<std::string, std::string> Channel::getTags() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_tags;
}

ChannelValue_t Channel::getValue(double time) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::lower_bound(m_time.begin(), m_time.end(), time);
    if (it != m_time.end()) {
        size_t index = std::distance(m_time.begin(), it);
        return m_value[index];
    }
    throw std::out_of_range("Time not found");
}
std::vector<double> Channel::getTimes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_time;
}
std::vector<ChannelValue_t> Channel::getValues() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_value;
}

bool Channel::isDataSorted() const {
    std::lock_guard<std::mutex> loc(m_mutex);
    return m_is_sorted;
}

// Sort data based on time with thread safety
void Channel::sortData() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_is_sorted) {
        // Step 1: Create indices and sort them based on time values
        std::vector<size_t> indices(m_time.size());
        for (size_t i = 0; i < indices.size(); ++i) {
            indices[i] = i;
        }
        std::sort(indices.begin(), indices.end(), [this](size_t a, size_t b) {
            return m_time[a] < m_time[b];
        });

        // Step 2: Initialize sorted_time and sorted_value based on sorted indices
        std::vector<double> sorted_time;
        std::vector<ChannelValue_t> sorted_value;
        sorted_time.reserve(m_time.size());
        sorted_value.reserve(m_value.size());
        for (size_t idx : indices) {
            sorted_time.emplace_back(m_time[idx]);
            sorted_value.emplace_back(m_value[idx]);
        }

        // Step 3: Combine duplicate times by averaging their values
        std::vector<double> new_time;
        std::vector<ChannelValue_t> new_value;
        new_time.reserve(sorted_time.size());
        new_value.reserve(sorted_value.size());

        size_t i = 0;
        while (i < sorted_time.size()) {
            double current_time = sorted_time[i];
            double sum = std::visit([](auto&& arg) -> double { return static_cast<double>(arg); }, sorted_value[i]);
            size_t count = 1;

            // Collect all values with the same time
            while (i + 1 < sorted_time.size() && sorted_time[i + 1] == current_time) {
                ++i;
                sum += std::visit([](auto&& arg) -> double { return static_cast<double>(arg); }, sorted_value[i]);
                ++count;
            }

            // Compute the average value
            double average_value = sum / static_cast<double>(count);
            new_time.push_back(current_time);
            new_value.emplace_back(average_value);
            ++i;
        }

        // Step 4: Assign back to the member vectors
        m_time = std::move(new_time);
        m_value = std::move(new_value);
        m_is_sorted = true;
    }
}
