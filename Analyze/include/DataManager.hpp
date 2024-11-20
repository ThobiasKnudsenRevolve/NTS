#pragma once

#include <iostream>
#include <ctime>
#include <cstdlib>
#include <map>
#include <vector>
#include <algorithm>
#include <utility>
#include <stdexcept>
#include <limits>
#include <unordered_set>
#include <thread>
#include <mutex>
#include <chrono>
#include <type_traits>
#include <stdint.h>
#include <memory>
#include <string>
#include <iomanip>
#include <sstream>
#include <cxxabi.h>
#include "implot.h"  // Include ImPlot for plotting
#include "InfluxDBClient.hpp"

long long convertLogIdToTimestampMs(const std::string& datetime_str) {
    // Expected format: "CAN_YYYY-MM-DD(HHMMSS)"
    struct tm tm_time = {0};
    int year, month, day, hour, minute, second;
    if (sscanf(datetime_str.c_str(), "CAN_%4d-%2d-%2d(%2d%2d%2d)",
               &year, &month, &day, &hour, &minute, &second) != 6) {
        // Handle parsing error
        std::cerr << "Invalid datetime format: " << datetime_str << std::endl;
        return -1;
    }
    tm_time.tm_year = year - 1900;  // tm_year is years since 1900
    tm_time.tm_mon  = month - 1;     // tm_mon is months since January (0-11)
    tm_time.tm_mday = day;
    tm_time.tm_hour = hour;
    tm_time.tm_min  = minute;
    tm_time.tm_sec  = second;
    tm_time.tm_isdst = -1;
    char* old_tz = getenv("TZ");
    char* old_tz_copy = nullptr;
    if (old_tz) {
        old_tz_copy = strdup(old_tz); // Duplicate the string to restore later
    }
    setenv("TZ", "UTC", 1);
    tzset();
    time_t timestamp = mktime(&tm_time);
    if (old_tz_copy) {
        setenv("TZ", old_tz_copy, 1);
        free(old_tz_copy);
    } else {
        unsetenv("TZ");
    }
    tzset();
    if (timestamp == -1) {
        std::cerr << "Failed to convert tm structure to time_t." << std::endl;
        return -1;
    }
    long long timestamp_ms = static_cast<long long>(timestamp) * 1000;
    return timestamp_ms;
}
inline std::string Demangle(const char* name) {
    int status = -1;
    std::unique_ptr<char, void(*)(void*)> res {
        abi::__cxa_demangle(name, NULL, NULL, &status),
        std::free
    };
    return (status == 0) ? res.get() : name;
}

class CommonMembersChannel {
public:

    std::string                 m_log_id;
    std::string                 m_measure;
    std::string                 m_field;
    
    std::string                 m_car = "";
    std::string                 m_driver = "";
    std::string                 m_event = "";
    std::string                 m_competition = "";
    std::string                 m_log_type = "";
    std::vector<std::string>    m_tags = {};


    CommonMembersChannel(const std::string& log_id, const std::string& measure, const std::string& field)
        : m_log_id(log_id),
          m_measure(measure),
          m_field(field)
    {}
    CommonMembersChannel(const CommonMembersChannel& other) 
        : m_log_id(other.m_log_id),
          m_measure(other.m_measure),
          m_field(other.m_field),

          m_car(other.m_car),
          m_driver(other.m_driver),
          m_event(other.m_event),
          m_competition(other.m_competition),
          m_log_type(other.m_log_type),
          m_tags(other.m_tags)
    {}
    virtual                                 ~CommonMembersChannel() {}
    virtual std::string                     GetLogId() const = 0;
    virtual std::string                     GetMeasurement() const = 0;
    virtual std::string                     GetField() const = 0;
    virtual std::vector<std::string>        GetTags() const = 0;
    virtual void                            PrintData() const = 0;
    virtual std::unique_ptr<CommonMembersChannel>    Clone() const = 0;
    virtual std::string                     GetTypeName() const = 0;
    virtual size_t                          GetDataPointCount() const = 0;
    virtual float                           GetMinTimeView() const = 0;
    virtual float                           GetMaxTimeView() const = 0;
    virtual float                           GetMinValueView() const = 0;
    virtual float                           GetMaxValueView() const = 0;
    virtual void                            AppendDataFrom(const CommonMembersChannel& other) = 0;
    virtual void                            writeToInfluxDB(InfluxDBClient& client) const = 0;
    virtual void                            postPlot() const = 0;
};

template <typename T>
class Channel : public CommonMembersChannel {
    static_assert(std::is_arithmetic<T>::value, "Channel requires a numeric type.");

public:

    mutable std::vector<double> m_time = {};
    mutable std::vector<T>      m_value = {};

    mutable bool                m_is_prepared = false;
    bool                        m_updated = true;
    bool                        m_follow_data = true;

    float                       m_min_time_view = -1.f;
    float                       m_max_time_view = 1.f;
    float                       m_min_value_view = -1.f;
    float                       m_max_value_view = 1.f;
    
    mutable std::mutex          m_data_mutex;

public:
    Channel(const std::string& log_id, const std::string& measure, const std::string& field) 
        : CommonMembersChannel(log_id, measure, field)
    {}
    Channel(const Channel& other) 
        : CommonMembersChannel(other),

          m_time(other.m_time),
          m_value(other.m_value),
          m_is_prepared(other.m_is_prepared),
          m_updated(other.m_updated),
          m_follow_data(other.m_follow_data),

          m_min_time_view(other.m_min_time_view),
          m_max_time_view(other.m_max_time_view),
          m_min_value_view(other.m_min_value_view),
          m_max_value_view(other.m_max_value_view) 
    {}
    Channel& operator=(const Channel&) = delete;
    Channel(Channel&&) = delete;
    Channel& operator=(Channel&&) = delete;
    std::unique_ptr<CommonMembersChannel> Clone() const override {
        // Lock the mutex to ensure thread safety during copying
        std::lock_guard<std::mutex> lock(m_data_mutex);
        return std::make_unique<Channel<T>>(*this);
    }
    std::string                 GetLogId() const override { return m_log_id; }
    std::string                 GetMeasurement() const override { return m_measure; }
    std::string                 GetField() const override { return m_field; }
    std::vector<std::string>    GetTags() const override { return m_tags; }
    std::string                 GetTypeName() const override { return Demangle(typeid(T).name()); }
    size_t                      GetDataPointCount() const override { std::lock_guard<std::mutex> lock(m_data_mutex); return m_time.size(); }
    float                       GetMinTimeView() const override { return m_min_time_view; }
    float                       GetMaxTimeView() const override { return m_max_time_view; }
    float                       GetMinValueView() const override { return m_min_value_view; }
    float                       GetMaxValueView() const override { return m_max_value_view; }
    void AppendDataFrom(const CommonMembersChannel& other) override {
        const Channel<T>* other_channel = dynamic_cast<const Channel<T>*>(&other);
        if (other_channel) {
            std::lock_guard<std::mutex> lock(this->m_data_mutex);
            std::lock_guard<std::mutex> other_lock(other_channel->m_data_mutex);
            this->m_time.insert(this->m_time.end(), other_channel->m_time.begin(), other_channel->m_time.end());
            this->m_value.insert(this->m_value.end(), other_channel->m_value.begin(), other_channel->m_value.end());
            this->m_is_prepared = false;
            this->m_updated = true;
        } else {
            // Types do not match
            std::cerr << "Cannot append data from channel of different type" << std::endl;
        }
    }
    void AddDatapoint(const std::pair<double, T>& new_point) {
        std::lock_guard<std::mutex> lock(m_data_mutex);
        if (!m_time.empty() && new_point.first <= m_time.back())
            m_is_prepared = false;
        m_time.emplace_back(new_point.first);
        m_value.emplace_back(new_point.second);
        m_updated = true;
    }
    void AddDatapoints(const std::vector<std::pair<double, T>>& newPoints) {
        std::lock_guard<std::mutex> lock(m_data_mutex);
        m_time.reserve(m_time.size() + newPoints.size());
        m_value.reserve(m_value.size() + newPoints.size());
        for (const auto& p : newPoints) {
            m_time.emplace_back(p.first);
            m_value.emplace_back(p.second);
        }
        m_is_prepared = false;
        m_updated = true;
    }
    void PrepareData() const {
        std::lock_guard<std::mutex> lock(m_data_mutex);
        if (!m_is_prepared) {
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
            std::vector<T> sorted_value; // Use T to maintain type consistency
            sorted_time.reserve(m_time.size());
            sorted_value.reserve(m_value.size());
            for (size_t idx : indices) {
                sorted_time.emplace_back(m_time[idx]);
                sorted_value.emplace_back(m_value[idx]);
            }

            // Step 3: Combine duplicate times by averaging their values
            std::vector<double> new_time;
            std::vector<T> new_value;
            size_t i = 0;
            while (i < sorted_time.size()) {
                double current_time = sorted_time[i];
                T sum_value = sorted_value[i];
                size_t count = 1;
                // Collect all values with the same time
                while (i + 1 < sorted_time.size() && sorted_time[i + 1] == current_time) {
                    ++i;
                    sum_value += sorted_value[i];
                    ++count;
                }
                // Compute the average value
                T average_value = sum_value / static_cast<T>(count);
                new_time.push_back(current_time);
                new_value.push_back(average_value);
                ++i;
            }

            // Step 4: Assign back to the member vectors
            m_time = std::move(new_time);
            m_value = std::move(new_value);
            m_is_prepared = true;
        }
    }
    T GetValue(double query_time) {
        if (!m_is_prepared) 
            PrepareData();
        if (m_time.empty()) 
            throw std::runtime_error("No data points available for interpolation.");
        if (query_time <= m_time.front()) 
            return m_value.front();
        if (query_time >= m_time.back()) 
            return m_value.back();
        // Binary search to find the upper bound
        size_t left = 0;
        size_t right = m_time.size();
        while (left < right) {
            size_t mid = left + (right - left) / 2;
            if (m_time[mid] < query_time)
                left = mid + 1;
            else
                right = mid;
        }

        if (left == m_time.size())
            throw std::runtime_error("Interpolation failed: upper bound not found.");
        if (m_time[left] == query_time)
            return m_value[left];
        
        size_t lower_idx = left - 1;
        size_t upper_idx = left;
        
        float t1 = static_cast<float>(m_time[lower_idx]);
        float v1 = static_cast<float>(m_value[lower_idx]);
        float t2 = static_cast<float>(m_time[upper_idx]);
        float v2 = static_cast<float>(m_value[upper_idx]);
        
        // Linear interpolation
        return v1 + (v2 - v1) * (query_time - t1) / (t2 - t1);
    }
    void PrintData() const override {
        std::lock_guard<std::mutex> lock(m_data_mutex);
        for (size_t i = 0; i < m_time.size(); ++i) {
            std::cout << "Time: " << m_time[i] << ", Value: " << m_value[i] << '\n';
        }
    }
    void GetDataForPlot(std::vector<double>& out_time, std::vector<T>& out_value) const {
        std::lock_guard<std::mutex> lock(m_data_mutex);
        out_time = m_time;
        out_value = m_value;
    }
    void postPlot() const override {
        std::lock_guard<std::mutex> lock(m_data_mutex);

        if (m_time.empty() || m_value.empty()) {
            return;
        }

        double first_time = m_time.front();
        std::vector<double> plot_time_double(m_time.size());
        for (size_t i = 0; i < m_time.size(); ++i) {
            plot_time_double[i] = (m_time[i] - first_time) / 1000000.0;
        }

        // Convert value to double
        std::vector<double> value_double(m_value.size());
        for (size_t i = 0; i < m_value.size(); ++i) {
            value_double[i] = static_cast<double>(m_value[i]);
        }

        ImPlot::PlotLine(m_field.c_str(), plot_time_double.data(), value_double.data(), static_cast<int>(plot_time_double.size()));
    }
    void writeToInfluxDB(InfluxDBClient& client) const override {

        if (!m_is_prepared) {
            PrepareData();
        }

        std::lock_guard<std::mutex> lock(m_data_mutex);

        if (m_time.empty() || m_value.empty()) {
            return;
        }
        
        long long millis_now = convertLogIdToTimestampMs(m_log_id);

        size_t batch_size = 5000;
        size_t total_points = m_time.size();
        size_t num_batches = (total_points + batch_size - 1) / batch_size;
        for (size_t batch = 0; batch < num_batches; ++batch) {
            size_t start_idx = batch * batch_size;
            size_t end_idx = std::min(start_idx + batch_size, total_points);
            std::ostringstream oss;
            for (size_t i = start_idx; i < end_idx; ++i) {
                oss << m_measure;
                if (m_car != "") {oss << ",log_id=" << m_log_id;}
                if (m_car != "") {oss << ",car=" << m_car;}
                if (m_driver != "") {oss << ",driver=" << m_driver;}
                if (m_event != "") {oss << ",event=" << m_event;}
                if (m_competition != "") {oss << ",competition=" << m_competition;}
                if (m_log_type != "") {oss << ",log_type=" << m_log_type;}
                oss << " ";
                oss << m_field << "=" << FormatValue(m_value.at(i)) << " ";
                oss << millis_now + static_cast<long long>(m_time[i]/1000.f) << "\n";
            }
            std::string data = oss.str();
            client.postToInfluxDB("CAN_Car", data);
        }
    }


private:
    // Helper function to escape special characters in measurement, tag keys/values
    std::string EscapeString(const std::string& str) const {
        std::string escaped;
        for (char c : str) {
            if (c == ',' || c == '=' || c == ' ' || c == '\\') {
                escaped += '\\';
            }
            escaped += c;
        }
        return escaped;
    }

    // Helper function to format field values
    std::string FormatValue(const T& val) const {
        if constexpr (std::is_same<T, std::string>::value) {
            // Escape double quotes and backslashes
            std::string escaped = val;
            size_t pos = 0;
            while ((pos = escaped.find_first_of("\\\"", pos)) != std::string::npos) {
                escaped.insert(pos, "\\");
                pos += 2;
            }
            return "\"" + escaped + "\"";
        } else if constexpr (std::is_same<T, bool>::value) {
            return val ? "true" : "false";
        } else if constexpr (std::is_integral<T>::value) {
            return std::to_string(val) + "i"; // Append 'i' to indicate integer
        } else if constexpr (std::is_floating_point<T>::value) {
            return std::to_string(val);
        } else {
            // Default case
            return std::to_string(val);
        }
    }

    std::string unixToISO8601(long long unixTimestamp) {
        std::chrono::system_clock::time_point tp = std::chrono::system_clock::from_time_t(unixTimestamp);
        std::time_t tt = std::chrono::system_clock::to_time_t(tp);
        std::tm tm = *gmtime(&tt);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            tp.time_since_epoch()) % 1000;
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
        oss << '.' << std::setw(3) << std::setfill('0') << ms.count() << 'Z';   
        return oss.str();
    }
};

class DataManager {
public:
    std::vector<std::unique_ptr<CommonMembersChannel>> channels;
    mutable std::mutex channels_mutex;  // Mutex to protect channels

public:

    template <typename T>
    Channel<T>* GetChannelPtr(const std::string& log_id, const std::string& measure, const std::string& field) {

        std::lock_guard<std::mutex> lock(channels_mutex);

        for (const auto& channel_ptr : channels) {
            if(channel_ptr->GetMeasurement() == measure 
            && channel_ptr->GetField() == field
            && channel_ptr->GetLogId() == log_id)
            {
                Channel<T>* typed_channel_ptr = dynamic_cast<Channel<T>*>(channel_ptr.get());
                if (typed_channel_ptr)
                    return typed_channel_ptr;
            }
        }
        return nullptr;
    }

    std::vector<CommonMembersChannel*> GetCommonMembersChannelPtrs(const std::string& log_id) {
        std::lock_guard<std::mutex> lock(channels_mutex);
        std::vector<CommonMembersChannel*> return_value = {};
        for (const auto& channel_ptr : channels) {
            if(channel_ptr->GetLogId() == log_id) {
                CommonMembersChannel* typed_channel_ptr = channel_ptr.get();
                if (typed_channel_ptr) {
                    return_value.push_back(typed_channel_ptr);
                }
                else {
                    printf("found a CommonMembersChannel is nullptr\n");
                    exit(-1);
                }
            }
        }
        return return_value;
    }

    CommonMembersChannel* GetCommonMembersChannelPtr_NoLock(const std::string& log_id, const std::string& measure, const std::string& field) {
        for (const auto& channel_ptr : channels) {
            if(channel_ptr->GetMeasurement() == measure 
            && channel_ptr->GetField() == field
            && channel_ptr->GetLogId() == log_id)  
            {
                return channel_ptr.get();
            }
        }
        return nullptr;
    }

    template <typename T>
    bool AddDatapoint(const std::string& log_id, const std::string& measure, const std::string& field, double time, T value) {
        Channel<T>* channel_ptr = GetChannelPtr<T>(log_id, measure, field);
        if (!channel_ptr) {
            channel_ptr = CreateNewChannel<T>(log_id, measure, field);
        }
        if (!channel_ptr) {
            // Error handling
            printf("ERROR: Could not create or retrieve channel.\n");
            return false;
        }
        // Add data point
        channel_ptr->AddDatapoint({time, value});
        return true;
    }

    void PrintData() const {
        std::lock_guard<std::mutex> lock(channels_mutex);  // Lock the mutex
        for (const auto& channel_ptr : channels) {
            channel_ptr->PrintData();
        }
    }

    DataManager& operator+=(const DataManager& other) {
        // Lock both mutexes without deadlock
        std::scoped_lock lock(this->channels_mutex, other.channels_mutex);

        for (const auto& other_channel_ptr : other.channels) {
            if (other_channel_ptr) {
                // Get log_id and tags from other_channel_ptr
                std::string log_id = other_channel_ptr->GetLogId();
                std::string measure = other_channel_ptr->GetMeasurement();
                std::string field = other_channel_ptr->GetField();

                // Attempt to find a matching channel in 'this'
                CommonMembersChannel* existing_channel_ptr = GetCommonMembersChannelPtr_NoLock(log_id, measure, field);

                if (existing_channel_ptr) {
                    // If channel exists, attempt to append data
                    if (existing_channel_ptr->GetTypeName() == other_channel_ptr->GetTypeName()) {
                        existing_channel_ptr->AppendDataFrom(*other_channel_ptr);
                    } else {
                        printf("uhhhh??\n");
                    }
                } else {
                    // Channel does not exist, clone and add the channel
                    std::unique_ptr<CommonMembersChannel> cloned_channel = other_channel_ptr->Clone();
                    channels.emplace_back(std::move(cloned_channel));
                }
            }
        }

        return *this;
    }


    void PrintMetadata() const {
        std::lock_guard<std::mutex> lock(channels_mutex);
        std::cout << "==================== Channels Metadata ====================\n";

        // Define column widths
        const int width_log_id = 30;
        const int width_measurement = 15;
        const int width_field = 30;
        const int width_type = 15;
        const int width_data_points = 15;

        // Print header
        std::cout << std::left 
                  << std::setw(width_log_id) << "Log ID" 
                  << std::setw(width_measurement) << "Measurement" 
                  << std::setw(width_field) << "Field" 
                  << std::setw(width_type) << "Type" 
                  << std::setw(width_data_points) << "Data Points" 
                  << "\n";

        std::cout << std::string(width_log_id + width_measurement + width_field + width_type + width_data_points, '-') << "\n";

        // Iterate through channels and print metadata
        for (const auto& channel_ptr : channels) {
            // Concatenate tags into a single string
            std::string tags_combined;
            for (const auto& tag : channel_ptr->GetTags()) {
                tags_combined += tag + " ";
            }
            if (!tags_combined.empty()) {
                tags_combined.pop_back(); // Remove trailing space
            }

            // Print each channel's metadata
            std::cout << std::left 
                      << std::setw(width_log_id) << channel_ptr->GetLogId() 
                      << std::setw(width_measurement) << channel_ptr->GetMeasurement()
                      << std::setw(width_field) << channel_ptr->GetField()
                      << std::setw(width_type) << channel_ptr->GetTypeName() 
                      << std::setw(width_data_points) << channel_ptr->GetDataPointCount() 
                      << "\n";
        }

        std::cout << "============================================================\n";
    }

    void writeToInfluxDB(InfluxDBClient& client) {
        std::lock_guard<std::mutex> lock(channels_mutex);

        for (const auto& channel_ptr : channels) {
            if (channel_ptr) {
                channel_ptr->writeToInfluxDB(client);
            }
            else {
                printf("channel_ptr is nullptr\n");
                exit(-1);
            }
        }
    }

private:

    template <typename T>
    Channel<T>* CreateNewChannel(const std::string& log_id, const std::string& measure, const std::string& field) {
        auto new_channel = std::make_unique<Channel<T>>(log_id, measure, field);
        Channel<T>* new_channel_ptr = new_channel.get();
        std::lock_guard<std::mutex> lock(channels_mutex); 
        channels.push_back(std::move(new_channel));
        return new_channel_ptr;
    }

};
