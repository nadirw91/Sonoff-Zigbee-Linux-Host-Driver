#ifndef ZSTACK_FRAME_H
#define ZSTACK_FRAME_H

#include "ZStackProtocol.h"
#include <vector>
#include <cstdint> // For uint8_t

namespace ZStack {
    class ZStackFrame {
        public:
            ZStackFrame();
            ZStackFrame(uint8_t cmd0, uint8_t cmd1, const std::vector<uint8_t>& payload = {});

            void setCommand(uint8_t c0, uint8_t c1);
            void setPayload(const std::vector<uint8_t>& payload);
    
            uint8_t getCommand0() const { return cmd0; }
            uint8_t getCommand1() const { return cmd1; }
            const std::vector<uint8_t>& getPayload() const { return payload; }

            std::vector<uint8_t> toSerialBytes() const;

            void print() const;

        private:
            uint8_t cmd0;
            uint8_t cmd1;
            std::vector<uint8_t> payload;

            uint8_t calculateChecksum() const;
    };
}

#endif // ZSTACK_FRAME_H