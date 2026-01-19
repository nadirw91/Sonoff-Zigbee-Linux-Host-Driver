#include "ZStackParser.h"
#include <iostream>
#include "Logger.h"

namespace ZStack {
    Parser::Parser() : currentState(State::WAITING_START), calculateChecksum(0) {}

    std::optional<ZStackFrame> Parser::parseByte(uint8_t byte) {
        switch(currentState) {
            case State::WAITING_START:
                if (byte == 0xFE) {
                    LOG_DEBUG << "[Parser] Found Start Byte. Resetting." << std::endl;
                    currentState = State::WAITING_LEN;
                    calculateChecksum = 0;
                    incomingPayload.clear();
                }
                break;
            case State::WAITING_LEN:
                incomingLen = byte;
                calculateChecksum ^= byte;
                currentState = State::WAITING_CMD0;
                LOG_DEBUG << "[Parser] Length set to: " << std::dec << (int)incomingLen << std::endl;
                break;
            case State::WAITING_CMD0:
                incomingCmd0 = byte;
                calculateChecksum ^= byte;
                currentState = State::WAITING_CMD1;
                
                break;
            case State::WAITING_CMD1:
                incomingCmd1 = byte;
                calculateChecksum ^= byte;

                if (incomingLen > 0) {
                    currentState = State::READING_DATA;
                    incomingPayload.reserve(incomingLen);
                } else {
                    currentState = State::WAITING_FCS;
                }
                break;
            case State::READING_DATA:
                incomingPayload.push_back(byte);
                calculateChecksum ^= byte;

                if (incomingPayload.size() >= incomingLen) {
                    currentState = State::WAITING_FCS;
                }

                break;
            case State::WAITING_FCS:
            // --- DEBUG START ---
                LOG_DEBUG << "DEBUG: Calced Checksum: " << std::hex << (int)calculateChecksum  << " vs Received: " << (int)byte << std::endl;
            // --- DEBUG END ---
                if (calculateChecksum == byte) {
                    // SUCCESS
                    currentState = State::WAITING_START;
                    return ZStackFrame(incomingCmd0, incomingCmd1, incomingPayload);
                } else {
                    // FAILED 
                    LOG_DEBUG << "[ERROR] Checksum Mismatch! Calculated: " << std::hex << (int)calculateChecksum << " Expected: " << (int)byte << std::endl;

                    currentState = State::WAITING_START;
                }
        }

        return std::nullopt;
    }
}