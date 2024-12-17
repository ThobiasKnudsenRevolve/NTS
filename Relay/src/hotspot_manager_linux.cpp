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

            // Check if NetworkManager is installed
            if (std::system("which nmcli > /dev/null 2>&1") != 0) {
                throw std::runtime_error("nmcli is not installed");
            }

            // Create the hotspot
            std::string createHotspotCmd = "nmcli dev wifi hotspot ifname wlan0 ssid " + ssid + " password " + password;
            if (std::system(createHotspotCmd.c_str()) != 0) {
                throw std::runtime_error("Failed to create hotspot using nmcli");
            }

            std::cout << "Hotspot created successfully on Linux.\n";
        } catch (const std::exception& e) {
            std::cerr << "Error creating hotspot on Linux: " << e.what() << "\n";
            throw;
        }
    }

    void stopHotspot() override {
        try {
            std::cout << "Stopping hotspot on Linux...\n";

            // Disconnect the hotspot
            std::string stopHotspotCmd = "nmcli connection down Hotspot";
            if (std::system(stopHotspotCmd.c_str()) != 0) {
                throw std::runtime_error("Failed to stop hotspot using nmcli");
            }

            std::cout << "Hotspot stopped successfully on Linux.\n";
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
