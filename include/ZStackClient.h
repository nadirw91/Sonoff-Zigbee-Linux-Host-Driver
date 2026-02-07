#ifndef ZSTACK_CLIENT_H
#define ZSTACK_CLIENT_H

#include <string>
#include <optional>
#include <memory>
#include <functional>
#include <iomanip>
#include "SerialPort.h"
#include "ZStackParser.h"
#include "ZStackProtocol.h"
#include "AFDataRequest.h"
#include "zdo/ZDOPacketParser.h"
#include "af/AFPacketParser.h"


namespace ZStack {
    struct SysVersion {
        uint8_t transport;
        uint8_t product;
        uint8_t major;
        uint8_t minor;
        uint8_t maint;
        uint32_t revision;
    };

    struct DeviceState {
        std::vector<uint8_t> iEEE_address;
        uint16_t short_address;
        uint8_t device_type;
        uint8_t state;
    };

    class ZStackClient {
        public:
            ZStackClient(const std::string& portName);
            ~ZStackClient();

            bool connect();
            void close();

            std::optional<SysVersion> getSystemVersion(int timeoutMs = 5000);

            void bindDevice(
                uint16_t targetShortAddr, 
                const std::vector<uint8_t>& targetIEEE,
                uint16_t clusterID, 
                const std::vector<uint8_t>& myIEEE
            );
            bool registerEndpoint();
            void process();
            void permitJoin(uint8_t durationSeconds);
            std::optional<DeviceState> getDeviceState();
            bool startNetwork();
            void reset();

            std::optional<ZStackFrame> sendAndWait(
                const ZStackFrame& request,
                uint8_t expectedCmd0,
                uint8_t expectedCmd1,
                int timeoutMs = 1000
            );

            static std::string ieeeToString(const std::vector<uint8_t>& ieeeBytes) {
                std::stringstream ss;
                ss << std::hex << std::setfill('0');
                // IEEE is usually Little Endian in packets, humans read Big Endian.
                // Let's store it as Big Endian (Reversed)
                for (int i = 7; i >= 0; i--) {
                    ss << std::setw(2) << (int)ieeeBytes[i];
                }
                
                return ss.str();
            }

            void setZdoPacketHandler(std::function<void(const ZDOPacket::Packet&)> handler) {
                zdoPacketHandler = handler;
            }

            void setAfPacketHandler(std::function<void(const AFPacket::Packet&)> handler) {
                afPacketHandler = handler;
            }

            void fetchActiveEndpoints(
                uint16_t targetShortAddr
            );

            void fetchSimpleDescriptor(
                uint16_t targetShortAddr,
                uint8_t endpoint
            );

        private:
            std::function<void(const ZDOPacket::Packet&)> zdoPacketHandler;
            std::function<void(const AFPacket::Packet&)> afPacketHandler;
            std::unique_ptr<SerialPort> serialPort;
            Parser parser;

            std::optional<ZStackFrame> waitForFrame(uint8_t expectedCmd0, 
                                                uint8_t expectedCmd1, 
                                                int timeoutMs);
            
            void routeFrameToParser(const ZStackFrame& frame);
            
            void send(const ZStackFrame& request);
    };
}

#endif // ZSTACK_CLIENT_H