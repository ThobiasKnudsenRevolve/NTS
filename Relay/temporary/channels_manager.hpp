#pragma once

#include "channel.hpp"
#include <vector>
#include <mutex>
#include <memory>
#include <unordered_map>
#include <string>

class ChannelsManager {
private:
    std::vector<std::unique_ptr<Channel>> channels;
    mutable std::mutex m_mutex;

public:
    ChannelsManager() = default;
    ~ChannelsManager() = default;

    // Disable copy semantics
    ChannelsManager(const ChannelsManager&) = delete;
    ChannelsManager& operator=(const ChannelsManager&) = delete;

    // Enable move semantics
    ChannelsManager(ChannelsManager&&) = default;
    ChannelsManager& operator=(ChannelsManager&&) = default;

    void addDatapoint(
        const std::string& log_id,
        const std::unordered_map<std::string, std::string>& tags,
        double time,
        const ChannelValue_t& value);

    void addDatapoints(
        const std::string& log_id,
        const std::unordered_map<std::string, std::string>& tags,
        const std::vector<double>& times,
        const std::vector<ChannelValue_t>& values);

    void addChannelsManager(const ChannelsManager& other);

    // Returns a pointer to the channel or nullptr if not found
    Channel* getChannelReference(const std::unordered_map<std::string, std::string>& tags);

private:
    void sortData();
    Channel* createChannel(const std::string& log_id, const std::unordered_map<std::string, std::string>& tags);
};
