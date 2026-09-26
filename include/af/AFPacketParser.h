#ifndef AF_PACKET_PARSER_H
#define AF_PACKET_PARSER_H
#include "ZStackFrame.h"

namespace AFPacket {
    enum AFResponseType : uint8_t {
        AF_INCOMING_MSG = 0x01
    };

    enum DeviceType: uint8_t {
        TEMPERATURE_SENSOR = 0x01,
        HUMIDITY_SENSOR = 0x02,
        BATTERY_SENSOR = 0x03,
        ACTION_PRESS = 0x04,
        SWITCH_DEVICE = 0x05,
        POWER_CONSUMPTION_DEVICE = 0x06,
        INSTANTANEOUS_POWER_CONSUMPTION_CLUSTER = 0x07,
        PRESENCE_SENSOR = 0x08,
        MOVEMENT_SENSOR = 0x09,
        TARGET_DISTANCE_SENSOR = 0x0A,
    };

    struct Packet {
        virtual ~Packet() = default; // Still need this for polymorphism!
        AFResponseType type;
    };

    struct DeviceReading {
        virtual ~DeviceReading() = default;
        DeviceType type;
    };

    struct TemperatureReading: public DeviceReading {
        uint16_t shortAddr;
        float temperatureReading;
        TemperatureReading()  {
            this->type = TEMPERATURE_SENSOR;
        }
    };


    struct HumidityReading: public DeviceReading {
        uint16_t shortAddr;
        float humidityReading;
        HumidityReading() {
            this->type = HUMIDITY_SENSOR;
        }
    };

    struct BatteryReading: public DeviceReading {
        uint16_t shortAddr;
        float batteryLevelReading;
        BatteryReading() {
            this->type = BATTERY_SENSOR;
        }
    };

    struct OnOffReading: public DeviceReading {
        uint16_t shortAddr;
        bool isOn;
        OnOffReading() {
            this->type = SWITCH_DEVICE;
        }
    };

    struct ButtonPressAction: public DeviceReading {
        ButtonPressAction(){
            this->type = ACTION_PRESS;
        }
    };

    struct PresenceReading: public DeviceReading {
        uint16_t shortAddr;
        bool present;
        PresenceReading() {
            this->type = PRESENCE_SENSOR;
        }
    };

    struct MovementReading: public DeviceReading {
        uint16_t shortAddr;
        uint8_t movement;
        MovementReading() {
            this->type = MOVEMENT_SENSOR;
        }
    };

    struct TargetDistanceReading: public DeviceReading {
        uint16_t shortAddr;
        float distanceMeters;
        TargetDistanceReading() {
            this->type = TARGET_DISTANCE_SENSOR;
        }
    };
    
    struct IncomingMessage : public Packet {
        uint16_t srcAddress;
        uint16_t clusterID;
        std::unique_ptr<DeviceReading> deviceReading;
        IncomingMessage() {
            this->type = AFPacket::AF_INCOMING_MSG;
        }
    };

    std::unique_ptr<AFPacket::Packet> parseZStackFrame(const ZStack::ZStackFrame& frame);
}

#endif