// channels_manager.cpp
#include "channels_manager.hpp"
#include <algorithm>
#include <iostream>

// Helper function to determine if two tags match (you can customize this as needed)
bool tagsMatch(const std::unordered_map<std::string, std::string>& a, const std::unordered_map<std::string, std::string>& b) {
    for (const auto& [key, value] : a) {
        auto it = b.find(key);
        if (it == b.end() || it->second != value) {
            return false;
        }
    }
    return true;
}

// Adds a single datapoint to the appropriate channel
void ChannelsManager::addDatapoint(
    const std::string& log_id,
    const std::unordered_map<std::string, std::string>& tags,
    double time,
    const ChannelValue_t& value) 
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Find the channel with the given log_id and tags
    Channel* channel_ptr = nullptr;
    for (auto& channel : channels) {
        if (channel->getLogId() == log_id && tagsMatch(tags, channel->getTags())) {
            channel_ptr = channel.get();
            break;
        }
    }

    // If channel not found, create a new one
    if (!channel_ptr) {
        channel_ptr = createChannel(log_id, tags);
        if (!channel_ptr) {
            std::cerr << "ERROR: Could not create or retrieve channel.\n";
            return;
        }
    }

    // Add the datapoint
    channel_ptr->addDatapoint(time, value);
}

// Adds multiple datapoints to the appropriate channel
void ChannelsManager::addDatapoints(
    const std::string& log_id,
    const std::unordered_map<std::string, std::string>& tags,
    const std::vector<double>& times,
    const std::vector<ChannelValue_t>& values) 
{
    if (times.size() != values.size()) {
        throw std::invalid_argument("times.size() != values.size()");
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // Find the channel with the given log_id and tags
    Channel* channel_ptr = nullptr;
    for (auto& channel : channels) {
        if (channel->getLogId() == log_id && tagsMatch(tags, channel->getTags())) {
            channel_ptr = channel.get();
            break;
        }
    }

    // If channel not found, create a new one
    if (!channel_ptr) {
        channel_ptr = createChannel(log_id, tags);
        if (!channel_ptr) {
            std::cerr << "ERROR: Could not create or retrieve channel.\n";
            return;
        }
    }

    // Add the datapoints
    channel_ptr->addDatapoints(times, values);
}

// Adds channels from another ChannelsManager
void ChannelsManager::addChannelsManager(const ChannelsManager& other) {
    // Lock both mutexes without deadlock
    std::scoped_lock lock(this->m_mutex, other.m_mutex);

    for (const auto& other_channel_ptr : other.channels) {
        if (other_channel_ptr) {
            // Get log_id and tags from other_channel_ptr
            std::string log_id = other_channel_ptr->getLogId();
            // Assuming tags are accessible (make m_tags public or provide a getter)
            // For this example, let's assume there's a getter
            // std::unordered_map<std::string, std::string> tags = other_channel_ptr->getTags();

            // Placeholder for tags retrieval
            std::unordered_map<std::string, std::string> tags; // Implement getTags()

            // Attempt to find a matching channel in 'this'
            Channel* existing_channel_ptr = nullptr;
            for (auto& channel : channels) {
                if (channel->getLogId() == log_id && tagsMatch(tags, channel->getTags())) {
                    existing_channel_ptr = channel.get();
                    break;
                }
            }

            if (existing_channel_ptr) {
                // Append data from the other channel
                existing_channel_ptr->addChannel(*other_channel_ptr);
            } else {
                // Clone and add the channel
                // Assuming Channel has a clone method or use copy constructor
                channels.emplace_back(std::make_unique<Channel>(*other_channel_ptr));
            }
        }
    }
}

// Retrieves a reference to a channel based on tags
Channel* ChannelsManager::getChannelReference(const std::unordered_map<std::string, std::string>& tags) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& channel_ptr : channels) {
        if (tagsMatch(tags, channel_ptr->getTags())) {
            return channel_ptr.get();
        }
    }
    return nullptr;
}

// Sorts data in all channels
void ChannelsManager::sortData() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& channel_ptr : channels) {
        channel_ptr->sortData();
    }
}

// Creates a new channel and adds it to the manager
Channel* ChannelsManager::createChannel(const std::string& log_id, const std::unordered_map<std::string, std::string>& tags) {
    auto new_channel = std::make_unique<Channel>(log_id, tags);
    Channel* channel_ptr = new_channel.get();
    channels.emplace_back(std::move(new_channel));
    return channel_ptr;
}
