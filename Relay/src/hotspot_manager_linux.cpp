#ifdef __linux__

#include "hotspot_manager.hpp"
#include <iostream>
#include <cstdlib>
#include <stdexcept>

// LinuxHotspotManager Implementation using nmcli
class LinuxHotspotManager : public HotspotManager {
public:
    void createHotspot(const std::string& ssid, const std::string& password) override {
        try {
            std::cout << "Creating hotspot on Linux...\n";
            
            if (std::system("which nmcli > /dev/null 2>&1") != 0) {
                throw std::runtime_error("nmcli is not installed");
            }

            std::string createHotspotCmd = "nmcli dev wifi hotspot ifname wlan0 ssid " + ssid + " password " + password;
            if (std::system(createHotspotCmd.c_str()) != 0) {
                throw std::runtime_error("Failed to create hotspot using nmcli");
            }

            // Retrieve and store the actual connection name
            FILE* pipe = popen("nmcli -t -f NAME c show --active | grep 'Hotspot'", "r");
            if (!pipe) throw std::runtime_error("Could not execute nmcli command to get hotspot name");
            char buffer[128];
            std::string hotspotName;
            if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                hotspotName = buffer;
                hotspotName.erase(hotspotName.find_last_not_of(" \n\r\t") + 1); // remove trailing whitespace
            }
            pclose(pipe);

            if (hotspotName.empty()) {
                throw std::runtime_error("Could not determine hotspot connection name");
            }
            m_hotspot_connection_name = hotspotName; // Store this for stopping later

            std::cout << "Hotspot created successfully on Linux. Connection name: " << m_hotspot_connection_name << "\n";
            m_hotspot_running = true;
        } catch (const std::exception& e) {
            std::cerr << "Error creating hotspot on Linux: " << e.what() << "\n";
            throw;
        }
    }
    void stopHotspot() override {
        try {
            if (m_hotspot_running) {
                std::cout << "Stopping hotspot on Linux...\n";
                std::string stopHotspotCmd = "nmcli connection down " + m_hotspot_connection_name;
                if (std::system(stopHotspotCmd.c_str()) != 0) {
                    throw std::runtime_error("Failed to stop hotspot using nmcli");
                }
                std::cout << "Hotspot stopped successfully on Linux.\n";
                m_hotspot_running = false;
            }
            else {
                std::cout << "There is no hotspot to stop.\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "Error stopping hotspot on Linux: " << e.what() << "\n";
            throw;
        }
    }
};

// Factory method implementation for Linux
std::unique_ptr<HotspotManager> HotspotManager::createInstance() {
    return std::make_unique<LinuxHotspotManager>();
}

#endif // __linux__
