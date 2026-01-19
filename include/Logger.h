#pragma once

#include <iostream>
#include <string>
#include <mutex>

enum class LogLevel {
    DEBUG = 0,
    INFO,
    WARN,
    ERROR,
    NONE // Use this to silence everything
};

class Logger {
public:
    // Global generic configuration
    static LogLevel currentLevel;

    static void setLevel(LogLevel level) {
        currentLevel = level;
    }

    // This checks if we should log BEFORE we do expensive formatting
    static bool enabled(LogLevel level) {
        return level >= currentLevel;
    }
};

// Initialize static member (usually in .cpp, but inline works for headers in C++17)
inline LogLevel Logger::currentLevel = LogLevel::INFO;

// The Helper Class: Acts like std::cout
class LogStream {
    LogLevel msgLevel;
    bool shouldLog;
public:
    LogStream(LogLevel level) : msgLevel(level) {
        shouldLog = Logger::enabled(level);
        
        if (shouldLog) {
            // Optional: Print a prefix
            switch (level) {
                case LogLevel::DEBUG: std::cout << "[DEBUG] "; break;
                case LogLevel::INFO:  std::cout << "[INFO]  "; break;
                case LogLevel::WARN:  std::cout << "[WARN]  "; break;
                case LogLevel::ERROR: std::cout << "[ERROR] "; break;
            }
        }
    }

    // Destructor prints the newline automatically
    ~LogStream() {
        if (shouldLog) {
            std::cout << std::endl; // Flush stream at end of line
        }
    }

    // Overload << to accept anything std::cout accepts
    template <typename T>
    LogStream& operator<<(const T& msg) {
        if (shouldLog) {
            std::cout << msg;
        }
        return *this;
    }

    // Support for manipulators like std::hex, std::setw
    LogStream& operator<<(std::ostream& (*os)(std::ostream&)) {
        if (shouldLog) {
            std::cout << os;
        }
        return *this;
    }
};

// Convenience Macros (To make usage look like a function)
#define LOG_DEBUG LogStream(LogLevel::DEBUG)
#define LOG_INFO  LogStream(LogLevel::INFO)
#define LOG_WARN  LogStream(LogLevel::WARN)
#define LOG_ERROR LogStream(LogLevel::ERROR)