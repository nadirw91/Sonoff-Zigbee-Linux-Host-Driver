#pragma once
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>

struct ZigbeeDevice {
    std::string ieee;       // Unique ID (e.g., "00124B0014D8A123")
    uint16_t shortAddr;     // Network Address (e.g., 0x16C5)
    std::string name;       // Friendly Name (e.g., "Living Room Sensor")
};

class DeviceManager {
private:
    std::string filename;
    
    // We keep two maps for fast lookup:
    // 1. Look up by IEEE (Stable ID)
    std::map<std::string, ZigbeeDevice> devicesByIEEE;
    
    // 2. Look up by Short Address (Runtime ID)
    std::map<uint16_t, std::string> shortToIEEE;

public:
    DeviceManager(const std::string& dbFile) : filename(dbFile) {
        load();
    }

    // Add or Update a device
    void addDevice(const std::string& ieee, uint16_t shortAddr) {
        // Check if we already know this device
        if (devicesByIEEE.find(ieee) == devicesByIEEE.end()) {
            // New Device! Give it a default name.
            std::cout << "[DeviceManager] New Device Discovered: " << ieee << std::endl;
            devicesByIEEE[ieee] = { ieee, shortAddr, "New Device" };
        } else {
            // Known Device: Just update the Short Address (it might have changed)
            devicesByIEEE[ieee].shortAddr = shortAddr;
        }

        // Update the lookup map
        shortToIEEE[shortAddr] = ieee;
        
        save(); // Save to disk immediately
    }

    // Rename a device (The user feature!)
    void renameDevice(const std::string& ieee, const std::string& newName) {
        if (devicesByIEEE.find(ieee) != devicesByIEEE.end()) {
            devicesByIEEE[ieee].name = newName;
            save();
        }
    }

    // Lookup a name using the Short Address (for incoming packets)
    std::string getName(uint16_t shortAddr) {
        if (shortToIEEE.find(shortAddr) != shortToIEEE.end()) {
            std::string ieee = shortToIEEE[shortAddr];
            return devicesByIEEE[ieee].name;
        }
        return "Unknown Device";
    }

    // Lookup the IEEE using Short Address (needed for binding usually)
    std::string getIEEE(uint16_t shortAddr) {
        if (shortToIEEE.find(shortAddr) != shortToIEEE.end()) {
            return shortToIEEE[shortAddr];
        }
        return "";
    }

private:
    void save() {
        std::ofstream file(filename);
        if (!file.is_open()) return;

        for (const auto& pair : devicesByIEEE) {
            const auto& dev = pair.second;
            // Format: IEEE,ShortAddr,Name
            file << dev.ieee << "," 
                 << std::hex << dev.shortAddr << "," 
                 << dev.name << std::endl;
        }
    }

    void load() {
        std::ifstream file(filename);
        if (!file.is_open()) return;

        std::string line;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string segment;
            std::vector<std::string> parts;

            // Split by comma
            while (std::getline(ss, segment, ',')) {
                parts.push_back(segment);
            }

            if (parts.size() >= 3) {
                std::string ieee = parts[0];
                // Convert Hex String back to integer
                uint16_t shortAddr = std::stoul(parts[1], nullptr, 16); 
                std::string name = parts[2];

                // Store in memory
                devicesByIEEE[ieee] = { ieee, shortAddr, name };
                shortToIEEE[shortAddr] = ieee;
            }
        }
        std::cout << "[DeviceManager] Loaded " << devicesByIEEE.size() << " devices." << std::endl;
    }
};