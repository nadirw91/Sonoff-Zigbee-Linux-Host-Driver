#ifndef ZSTACK_PARSER_H
#define ZSTACK_PARSER_H

#include <vector>
#include <optional>
#include "ZStackFrame.h"

namespace ZStack {

    class Parser {
        public:
            Parser();

            // Attempts to parse a Z-Stack frame from the given byte stream.
            // Returns an optional ZStackFrame if parsing is successful.
            std::optional<ZStackFrame> parseByte(uint8_t byte);
        
        private:
            enum class State {
                WAITING_START,  // Looking for 0xFE
                WAITING_LEN,    // Reading the length byte
                WAITING_CMD0,   // Reading first command byte
                WAITING_CMD1,   // Reading second command byte
                READING_DATA,   // Reading the payload
                WAITING_FCS     // Reading the checksum
            };
            
            State currentState;

            uint8_t incomingLen;
            uint8_t incomingCmd0;
            uint8_t incomingCmd1;
            std::vector<uint8_t> incomingPayload;

            uint8_t calculateChecksum;
    };
}

#endif // ZSTACK_PARSER_H