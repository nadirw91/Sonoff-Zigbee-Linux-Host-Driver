#ifndef ZDO_PACKET_PARSER_H
#define ZDO_PACKET_PARSER_H
#include "../ZStackFrame.h"

namespace ZDOPacket {
    enum ZDOResponseType : uint8_t {
        DEVICE_ANNOUNCEMENT = 0x01,
        DEVICE_DESCRIPTION = 0x02,
        ACTIVE_ENDPOINTS = 0x03,
        BIND_RESPONSE = 0x04
    };

    struct Packet {
        virtual ~Packet() = default; // Still need this for polymorphism!
        ZDOResponseType type;
    };

    struct DeviceAnnouncementResponse : public Packet {
        uint16_t networkAddress;
        uint16_t srcAddress;
        uint64_t ieeeAddress;
        DeviceAnnouncementResponse() {
            this->type = DEVICE_ANNOUNCEMENT;
        }
    };

    // Response for Simple Descriptor Request Packet
    struct DeviceDescriptionResponse : public Packet {
        uint16_t sourceAddress;
        uint16_t networkAddress;
        uint8_t endpoint;
        uint16_t profileID;
        uint16_t deviceID;
        std::vector<uint16_t> inputClusters;
        std::vector<uint16_t> outputClusters;
        DeviceDescriptionResponse() {
            this->type = DEVICE_DESCRIPTION;
        }
    };

    // Response for Active Endpoint Request Packet
    struct DeviceActiveEndpointResponse : public Packet {
        uint16_t networkAddress;
        uint16_t srcAddress;
        std::vector<uint8_t> activeEndpoints;
        DeviceActiveEndpointResponse() {
            this->type = ACTIVE_ENDPOINTS;
        }
    };

    struct BindRequestResponse: public Packet {
        uint16_t srcAddress;
        bool success;
        BindRequestResponse() {
            this->type = BIND_RESPONSE;
        }
    };

    std::unique_ptr<ZDOPacket::Packet> parseZStackFrame(const ZStack::ZStackFrame& frame);
};

#endif