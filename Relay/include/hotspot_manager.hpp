// hotspot_manager.hpp
#pragma once

#include <string>
#include <memory>
#include <stdexcept>

// Abstract base class for Hotspot Management
class HotspotManager {
public:
    virtual ~HotspotManager() = default;

    // Create a hotspot with the given SSID and password
    virtual void createHotspot(const std::string& ssid, const std::string& password) = 0;

    // Stop the hotspot
    virtual void stopHotspot() = 0;

    // Factory method to create the appropriate HotspotManager instance
    static std::unique_ptr<HotspotManager>  createInstance();

    bool m_hotspot_running = false;
    std::string m_hotspot_connection_name;
};
