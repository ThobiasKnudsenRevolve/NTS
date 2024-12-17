#ifdef _WIN32

#include "hotspot_manager.hpp"
#include <iostream>
#include <cstdlib>
#include <stdexcept>

// WindowsHotspotManager Implementation
class WindowsHotspotManager : public HotspotManager {
public:
    void createHotspot(const std::string& ssid, const std::string& password) override {
        try {
            std::cout << "Creating hotspot on Windows...\n";

            // Set hosted network mode to allow
            std::string setModeCmd = "netsh wlan set hostednetwork mode=allow ssid=" + ssid + " key=" + password;
            if (std::system(setModeCmd.c_str()) != 0) {
                throw std::runtime_error("Failed to set hostednetwork mode");
            }

            // Start the hosted network
            std::string startNetworkCmd = "netsh wlan start hostednetwork";
            if (std::system(startNetworkCmd.c_str()) != 0) {
                throw std::runtime_error("Failed to start hosted network");
            }

            std::cout << "Hotspot created successfully on Windows.\n";
        } catch (const std::exception& e) {
            std::cerr << "Error creating hotspot on Windows: " << e.what() << "\n";
            throw;
        }
    }

    void stopHotspot() override {
        try {
            std::cout << "Stopping hotspot on Windows...\n";

            // Stop the hosted network
            std::string stopNetworkCmd = "netsh wlan stop hostednetwork";
            if (std::system(stopNetworkCmd.c_str()) != 0) {
                throw std::runtime_error("Failed to stop hosted network");
            }

            std::cout << "Hotspot stopped successfully on Windows.\n";
        } catch (const std::exception& e) {
            std::cerr << "Error stopping hotspot on Windows: " << e.what() << "\n";
            throw;
        }
    }
};

// Factory method implementation for Windows
std::unique_ptr<HotspotManager> HotspotManager::createInstance() {
    return std::make_unique<WindowsHotspotManager>();
}

#endif // _WIN32
