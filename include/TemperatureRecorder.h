#pragma once
#include <ctime>
#include <iomanip>
#include <chrono>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>

class TemperatureRecorder {
private:
    std::string filename;

public:
    TemperatureRecorder(const std::string& dbFile) : filename(dbFile) {
        
    }

    // Add or Update a device
    void saveTemperatureReading(float temperature) {
        save(temperature); // Save to disk immediately
    }

    void saveHumidityReading(float humidity) {
        save(humidity); // Save to disk immediately
    }

private:
    void save(float temperature) {
        std::ofstream file(filename, std::ios::app);
        if (!file.is_open()) return;

        // For simplicity, we just append the temperature with a timestamp
        // In a real implementation, you would associate this with a device
        // Here we just write the temperature to the file.

        auto timestamp = getCurrentTime();

        file << temperature << ", " 
                 << timestamp << std::endl;
    }

    inline std::string getCurrentTime() {
        // 1. Get the current time from the system clock
        auto now = std::chrono::system_clock::now();
    
        // 2. Convert to classic C-style time (for formatting)
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    
        // 3. Format it
        std::stringstream ss;
        ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
};