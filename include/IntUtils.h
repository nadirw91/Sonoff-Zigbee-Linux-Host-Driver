#pragma once
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>

static uint64_t convertToInt64(std::vector<uint8_t> arr) {
    // Safety check (optional but recommended)
    if (arr.size() < 8) return 0; 

    uint64_t result = 0;

    // Cast to uint64_t BEFORE shifting
    result |= static_cast<uint64_t>(arr[0]);       // Shift 0
    result |= static_cast<uint64_t>(arr[1]) << 8;
    result |= static_cast<uint64_t>(arr[2]) << 16;
    result |= static_cast<uint64_t>(arr[3]) << 24;
    result |= static_cast<uint64_t>(arr[4]) << 32;
    result |= static_cast<uint64_t>(arr[5]) << 40;
    result |= static_cast<uint64_t>(arr[6]) << 48;
    result |= static_cast<uint64_t>(arr[7]) << 56; // Max shift is 56

    return result;
}