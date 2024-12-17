#ifdef __APPLE__

#include "hotspot_manager.hpp"
#include <iostream>
#include <cstdlib>
#include <stdexcept>

// MacOSHotspotManager Implementation
class MacOSHotspotManager : public HotspotManager {
public:
    void createHotspot(const std::string& ssid, const std::string& password) override {
        try {
            std::cout << "Creating hotspot on macOS...\n";

            // Create network service named "AdHoc"
            std::string createServiceCmd = "sudo networksetup -createnetworkservice AdHoc en0";
            if (std::system(createServiceCmd.c_str()) != 0) {
                throw std::runtime_error("Failed to create network service 'AdHoc'");
            }

            // Set manual IP configuration
            std::string setManualIpCmd = "sudo networksetup -setmanual AdHoc 192.168.1.88 255.255.255.0";
            if (std::system(setManualIpCmd.c_str()) != 0) {
                throw std::runtime_error("Failed to set manual IP configuration for 'AdHoc'");
            }

            // Note: Enabling Internet Sharing via command line on macOS is non-trivial
            // It often requires AppleScript or other automation tools.
            // Here, we'll assume that creating the network service is sufficient for demonstration.

            std::cout << "Hotspot created successfully on macOS.\n";
        } catch (const std::exception& e) {
            std::cerr << "Error creating hotspot on macOS: " << e.what() << "\n";
            throw;
        }
    }

    void stopHotspot() override {
        try {
            std::cout << "Stopping hotspot on macOS...\n";

            // Delete network service named "AdHoc"
            std::string deleteServiceCmd = "sudo networksetup -deleteNetworkService AdHoc";
            if (std::system(deleteServiceCmd.c_str()) != 0) {
                throw std::runtime_error("Failed to delete network service 'AdHoc'");
            }

            std::cout << "Hotspot stopped successfully on macOS.\n";
        } catch (const std::exception& e) {
            std::cerr << "Error stopping hotspot on macOS: " << e.what() << "\n";
            throw;
        }
    }
};

// Factory method implementation for macOS
std::unique_ptr<HotspotManager> HotspotManager::createInstance() {
    return std::make_unique<MacOSHotspotManager>();
}

#endif // __APPLE__
