#include "ZStackFrame.h"
#include <iostream>
#include <iomanip>
#include "Logger.h"

ZStack::ZStackFrame::ZStackFrame() : cmd0(0), cmd1(0), payload({}) {}

ZStack::ZStackFrame::ZStackFrame(uint8_t cmd0, uint8_t cmd1, const std::vector<uint8_t>& payload)
    : cmd0(cmd0), cmd1(cmd1), payload(payload) {}

void ZStack::ZStackFrame::setCommand(uint8_t c0, uint8_t c1) {
    cmd0 = c0;
    cmd1 = c1;
}

void ZStack::ZStackFrame::setPayload(const std::vector<uint8_t>& payload) {
    this->payload = payload;
}

uint8_t ZStack::ZStackFrame::calculateChecksum() const {
    uint8_t checksum = 0;
    uint8_t len = (uint8_t)payload.size();

    if (len > 0) {
        checksum ^= len;
    }

    checksum ^= cmd0;
    checksum ^= cmd1;

    for (uint8_t byte : payload) {
        checksum ^= byte;
    }
    return checksum;
}

std::vector<uint8_t> ZStack::ZStackFrame::toSerialBytes() const {
    std::vector<uint8_t> frame;

    // Start byte
    frame.push_back(0xFE);

    frame.push_back(payload.size()); // Length byte
    frame.push_back(cmd0);
    frame.push_back(cmd1);

    frame.insert(frame.end(), payload.begin(), payload.end());

    frame.push_back(calculateChecksum());

    return frame;
}

void ZStack::ZStackFrame::print() const {
    std::vector<uint8_t> frameBytes = toSerialBytes();

    LOG_DEBUG << "Z-Stack Frame: ";

    for (auto b : frameBytes) {
        LOG_DEBUG << std::hex << std::uppercase << std::setw(2) << (int)b << " ";
    }

    LOG_DEBUG << std::dec << std::endl; // Reset to decimal
}