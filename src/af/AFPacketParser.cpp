#include <iostream>
#include <thread>
#include <chrono>
#include <optional>
#include "ZStackFrame.h"
#include "af/AFPacketParser.h"
#include "IntUtils.h"
#include "Logger.h"

using namespace ZStack;

namespace AFPacket
{
    std::unique_ptr<AFPacket::Packet> parseZStackFrame(const ZStack::ZStackFrame &frame)
    {
        if (frame.getCommand0() == (ZStack::AREQ | ZStack::AF) &&
            frame.getCommand1() == ZStack::AF_INCOMING_MSG)
        {
            auto p = frame.getPayload();
            uint16_t srcAddr = p[4] | (p[5] << 8);

            LOG_DEBUG << ">>> AF_INCOMING_MSG SRC ADDRESS: " << std::hex << std::setw(2) << (int)srcAddr << std::endl;

            LOG_DEBUG << ">>> AF_INCOMING_MSG PAYLOAD SIZE: " << std::hex << std::setw(2) << (int)p.size() << std::endl;

            int dataOffset = 17; // Standard Header Size
            if (p.size() <= dataOffset)
                return nullptr;

            uint8_t zclCmd = p[dataOffset + 2];
            uint16_t incomingClusterID = p[2] | (p[3] << 8);

            LOG_DEBUG << ">>> [Config Response] From " << srcAddr
                      << " (Cluster " << getClusterName(incomingClusterID) << ") "
                      << "ZCL Cmd: " << getZCLCommandName(zclCmd) << std::endl;

            // A. CHECK FOR CONFIG RESPONSE (Receipt)
            // ------------------------------------------------
            // CASE A: CONFIGURATION RESPONSE (Receipt)
            // ------------------------------------------------
            if (zclCmd == 0x07)
            {
                uint8_t status = p[dataOffset + 3];
                if (status == 0x00)
                    LOG_DEBUG << "    Result: SUCCESS" << std::endl;
                else
                    LOG_DEBUG << "    Result: FAIL (Code " << std::hex << (int)status << ")" << std::endl;
            }

            // ------------------------------------------------
            // CASE B: TOGGLE COMMAND (Button Press) - MOVED HERE!
            // ------------------------------------------------
            else if (incomingClusterID == 0x0006 && zclCmd == 0x02)
            {
                LOG_DEBUG << ">>> [" << srcAddr << "] ACTION: Button Pressed (Toggle)" << std::endl;
                auto msg = std::make_unique<IncomingMessage>();
                    msg->srcAddress = srcAddr;
                    msg->clusterID = incomingClusterID;
                    msg->deviceReading = std::make_unique<ButtonPressAction>();
                    return msg;
            }

            // ------------------------------------------------
            // CASE C: SENSOR DATA (Report or Read Response)
            // ------------------------------------------------
            else if (zclCmd == 0x0A || zclCmd == 0x01)
            {
                auto deviceReading = parseDeviceReadingData(zclCmd, srcAddr, incomingClusterID, p);
                if (deviceReading) {
                    auto msg = std::make_unique<IncomingMessage>();
                    msg->srcAddress = srcAddr;
                    msg->clusterID = incomingClusterID;
                    msg->deviceReading = std::move(deviceReading);
                    return msg;
                }
            }
        } else {
            LOG_DEBUG << "[WARNING] AFPacketParser: Unknown Frame Cmd0: " << std::hex << (int)frame.getCommand0()
                      << " Cmd1: " << std::hex << (int)frame.getCommand1() << std::endl;
        }

        return nullptr;
    }

    static std::unique_ptr<AFPacket::DeviceReading> parseDeviceReadingData(uint8_t zclCmd, const uint16_t srcAddr, const uint16_t incomingClusterID, const std::vector<uint8_t> p)
    {
        uint8_t dataOffset = 17;

        // 1. CALCULATE OFFSETS
        // Reports (0x0A) start value at offset 6
        // Read Responses (0x01) have a Status byte, so value starts at offset 7
        int valueOffset = (zclCmd == 0x0A) ? 6 : 7;

        // For Read Responses, check Success (0x00) before reading
        if (zclCmd == 0x01 && p[dataOffset + 3] != 0x00)
        {
            LOG_DEBUG << ">>> [" << srcAddr << "] Read Failed (Status " << std::hex << (int)p[dataOffset + 3] << ")" << std::endl;
            return nullptr;
        }

        uint16_t attrID = p[dataOffset + 3] | (p[dataOffset + 4] << 8);

        // 2. READ DATA USING DYNAMIC OFFSET

        // Temp (Cluster 0x0402)
        if (incomingClusterID == 0x0402 && attrID == 0x0000)
        {
            int16_t rawValue = p[dataOffset + valueOffset] | (p[dataOffset + valueOffset + 1] << 8);
            float val = rawValue / 100.0f;
            LOG_DEBUG << ">>> [" << srcAddr << "] Temperature: " << std::fixed << std::setprecision(2) << val << " C" << std::endl;
            TemperatureReading tempDevice;
            tempDevice.shortAddr = srcAddr;
            tempDevice.temperatureReading = val;
            return std::make_unique<AFPacket::TemperatureReading>(tempDevice);
        }

        // Humidity (Cluster 0x0405)
        else if (incomingClusterID == 0x0405 && attrID == 0x0000)
        {
            int16_t rawValue = p[dataOffset + valueOffset] | (p[dataOffset + valueOffset + 1] << 8);
            float val = rawValue / 100.0f;
            LOG_DEBUG << ">>> [" << srcAddr << "] Humidity: " << std::fixed << std::setprecision(2) << val << " %" << std::endl;
            HumidityReading humidityDevice;
            humidityDevice.shortAddr = srcAddr;
            humidityDevice.humidityReading = val;
            return std::make_unique<AFPacket::HumidityReading>(humidityDevice);
        }

        // Battery (Cluster 0x0001)
        else if (incomingClusterID == 0x0001 && attrID == 0x0021)
        {
            // Battery is always 1 byte (uint8)
            uint8_t rawValue = p[dataOffset + valueOffset];
            float battery = rawValue / 2.0f;
            LOG_DEBUG << ">>> [" << srcAddr << "] Battery: " << std::fixed << std::setprecision(1) << battery << "%" << std::endl;
            BatteryReading batteryReading;
            batteryReading.shortAddr = srcAddr;
            batteryReading.batteryLevelReading = battery;
        } else {
            LOG_DEBUG << ">>> [" << srcAddr << "] Unknown Device Reading: Cluster " << std::hex << (int) incomingClusterID
                      << " AttrID " << std::hex << (int) attrID << std::endl;
        }

        return nullptr;
    }

}