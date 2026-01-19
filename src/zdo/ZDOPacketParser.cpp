#include <iostream>
#include <thread>
#include <chrono>
#include <optional>
#include "ZStackFrame.h"
#include "zdo/ZDOPacketParser.h"
#include "IntUtils.h"
#include "Logger.h"

using namespace ZStack;

namespace ZDOPacket {
    std::unique_ptr<ZDOPacket::Packet> parseZStackFrame(const ZStack::ZStackFrame& frame) {
            if (frame.getCommand0() == (AREQ | ZDO) && 
                frame.getCommand1() == ZDO_END_DEVICE_ANNCE_IND) {
                
                auto p = frame.getPayload();
                uint16_t srcAddr = p[0] | (p[1] << 8);
                uint16_t networkAddr = p[2] | (p[3] << 8);
                std::vector<uint8_t> ieeeBytes;
                for(int i=0; i<8; i++) ieeeBytes.push_back(p[4+i]);        
                
                auto deviceAnnouncement = std::make_unique<ZDOPacket::DeviceAnnouncementResponse>();
                deviceAnnouncement->networkAddress = networkAddr;
                deviceAnnouncement->srcAddress = srcAddr;
                deviceAnnouncement->ieeeAddress = convertToInt64(ieeeBytes);

                return deviceAnnouncement;
            } else if (frame.getCommand0() == (AREQ | ZDO) && 
                frame.getCommand1() == ZDO_BIND_RSP) {
                
                auto p = frame.getPayload();

                auto bindResponse = std::make_unique<ZDOPacket::BindRequestResponse>();
                bindResponse->srcAddress = p[0] | (p[1] << 8);
                bindResponse->success = p[2] == 0;

                return bindResponse;
            } else if (frame.getCommand0() == (AREQ | ZDO) && 
                frame.getCommand1() == ZDO_ACTIVE_EP_RSP) {
                
                auto p = frame.getPayload();
                LOG_INFO << ">>> ZDO PARSER PAYLOAD LENGTH: " << std::dec << p.size() << std::endl;

                uint16_t srcAddr = p[0] | (p[1] << 8);
                uint8_t status = p[2];
                LOG_INFO << ">>> ZDO PARSER PAYLOAD STATUS: " << std::hex << (int)status << std::endl;

                uint16_t networkAddr = p[3] | (p[4] << 8);
                uint8_t endpointCount = p[5];
                LOG_INFO << ">>> ZDO PARSER ACTIVE EP COUNT: " << std::dec << (int)endpointCount << std::endl;

                std::vector<uint8_t> activeEndpoints;
                for(int i=0; i<endpointCount; i++) {
                    activeEndpoints.push_back(p[6 + i]);
                }

                auto activeEpResponse = std::make_unique<ZDOPacket::DeviceActiveEndpointResponse>();
                activeEpResponse->networkAddress = networkAddr;
                activeEpResponse->srcAddress = srcAddr;
                activeEpResponse->activeEndpoints = activeEndpoints;

                return activeEpResponse;
            } else if (frame.getCommand0() == (AREQ | ZDO) && 
                frame.getCommand1() == ZDO_SIMPLE_DESC_RSP) {
                
                auto p = frame.getPayload();
                uint16_t srcAddr = p[0] | (p[1] << 8);
                uint8_t status = p[2];
                uint16_t networkAddr = p[3] | (p[4] << 8);
                uint8_t lengthOfDesc = p[5];
                uint8_t endpoint = p[6];
                uint16_t profileID = p[7] | (p[8] << 8);
                uint16_t deviceID = p[9] | (p[10] << 8);
                uint8_t deviceVersion = p[11];
                uint8_t inputClusterCount = p[12];
                int inputClusterOffset=13;
                std::vector<uint16_t> inputClusters;
                for(int i=0; i<inputClusterCount; i++) {
                    inputClusters.push_back(p[inputClusterOffset + i*2] | (p[(inputClusterOffset + 1) + i*2] << 8));
                }
                uint8_t outputClusterCount = p[inputClusterOffset + inputClusterCount*2];
                int outputClusterOffset = inputClusterOffset + inputClusterCount*2 + 1;
                std::vector<uint16_t> outputClusters;
                for(int i=0; i<outputClusterCount; i++) {
                    outputClusters.push_back(p[outputClusterOffset + i*2] | 
                                            (p[outputClusterOffset + 1 + i*2] << 8));
                }

                auto deviceDescResponse = std::make_unique<ZDOPacket::DeviceDescriptionResponse>();
                deviceDescResponse->sourceAddress = srcAddr;
                deviceDescResponse->networkAddress = networkAddr;
                deviceDescResponse->endpoint = endpoint;
                deviceDescResponse->profileID = profileID;
                deviceDescResponse->deviceID = deviceID;
                deviceDescResponse->inputClusters = inputClusters;
                deviceDescResponse->outputClusters = outputClusters;   

                return deviceDescResponse;
            }

        return nullptr;
    }
}
