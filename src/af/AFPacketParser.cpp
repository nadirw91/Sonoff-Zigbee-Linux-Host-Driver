#include <iostream>
#include <thread>
#include <chrono>
#include <optional>
#include "ZStackFrame.h"
#include "af/AFPacketParser.h"
#include "IntUtils.h"
#include "Logger.h"
#include "ZStackProtocol.h"

using namespace ZStack;

namespace
{
    // Returns the length of the data value based on ZCL Data Type
    // Returns -1 if variable length (like string) or unknown
    static int getDataTypeLength(uint8_t dataType)
    {
        switch (dataType)
        {
        case 0x10: // Boolean
        case 0x18: // Bitmap8
        case 0x20: // Uint8
        case 0x30: // Enum8
            return 1;
        case 0x21: // Uint16
        case 0x29: // Int16
        case 0x19: // Bitmap16
            return 2;
        case 0x23: // Uint32
        case 0x2B: // Int32
        case 0x39: // Single Precision Float
            return 4;
        case 0x42:     // Char String (First byte is length)
            return -1; // Special handling needed
        default:
            return 0; // Unknown
        }
    }

    std::unique_ptr<AFPacket::DeviceReading> parseDeviceReadingData(uint8_t zclCmd, const uint16_t srcAddr, const uint16_t incomingClusterID, const std::vector<uint8_t> &p)
    {
        // 1. Setup Offsets
        uint8_t dataOffset = 17; // Start of ZCL Frame
        if (p.size() < dataOffset + 3)
            return nullptr;

        // Define where the Attribute List starts inside the ZCL Frame
        // Reports (0x0A): Frame(3) + AttrList...
        // ReadRsp (0x01): Frame(3) + AttrList... (Status is inside the loop for ReadRsp)
        int currentIndex = dataOffset + 3;

        // 2. Loop through all attributes in the packet
        while (currentIndex + 2 < p.size())
        {

            uint16_t attrID = p[currentIndex] | (p[currentIndex + 1] << 8);
            currentIndex += 2; // Move past ID

            // For Read Response (0x01), there is a Status byte here
            if (zclCmd == 0x01)
            {
                uint8_t status = p[currentIndex];
                currentIndex++; // Move past Status
                if (status != 0x00)
                {
                    continue; // This attribute read failed, try the next one
                }
            }

            uint8_t dataType = p[currentIndex];
            currentIndex++; // Move past Type

            // Calculate Data Length
            int dataLength = getDataTypeLength(dataType);

            // Handle Variable Length Strings (0x42)
            if (dataLength == -1)
            {
                if (currentIndex < p.size())
                {
                    dataLength = p[currentIndex] + 1; // Length byte + N data bytes
                }
                else
                {
                    break; // Malformed
                }
            }

            // Safety Check: Do we have enough bytes left?
            if (currentIndex + dataLength > p.size())
                break;

            // ---------------------------------------------------------
            // 3. MATCHING LOGIC (Is this the attribute we want?)
            // ---------------------------------------------------------

            // Temperature (0x0402 -> 0x0000)
            if (incomingClusterID == ZStack::ClusterID::TEMPERATURE_MEASUREMENT_CLUSTER && attrID == 0x0000)
            {
                int16_t raw = p[currentIndex] | (p[currentIndex + 1] << 8);
                float val = raw / 100.0f;
                AFPacket::TemperatureReading t;
                t.shortAddr = srcAddr;
                t.temperatureReading = val;
                return std::make_unique<AFPacket::TemperatureReading>(t);
            }

            // Humidity (0x0405 -> 0x0000)
            else if (incomingClusterID == ZStack::ClusterID::HUMIDITY_MEASUREMENT_CLUSTER && attrID == 0x0000)
            {
                int16_t raw = p[currentIndex] | (p[currentIndex + 1] << 8);
                float val = raw / 100.0f;
                AFPacket::HumidityReading h;
                h.shortAddr = srcAddr;
                h.humidityReading = val;
                return std::make_unique<AFPacket::HumidityReading>(h);
            }

            // On/Off Switch (0x0006 -> 0x0000)
            else if (incomingClusterID == ZStack::ClusterID::ON_OFF_CLUSTER && attrID == 0x0000)
            {
                bool isOn = (p[currentIndex] == 1);
                LOG_DEBUG << ">>> [" << srcAddr << "] Switch: " << (isOn ? "ON" : "OFF") << std::endl;
                AFPacket::OnOffReading s;
                s.shortAddr = srcAddr;
                s.isOn = isOn;
                return std::make_unique<AFPacket::OnOffReading>(s);
            }

            // Electrical Measurement (0x0B04 -> Active Power 0x050B)
            else if (incomingClusterID == ZStack::ClusterID::POWER_CONSUMPTION_CLUSTER && attrID == 0x050B)
            {
                int16_t raw = p[currentIndex] | (p[currentIndex + 1] << 8);
                // Note: Real power often needs Multiplier/Divisor from other attributes!
                float watts = (float)raw;
                LOG_DEBUG << ">>> [" << srcAddr << "] Power: " << watts << "W" << std::endl;
                // Return your PowerReading struct here...
            }

            // 4. PREPARE FOR NEXT ITERATION
            // If we didn't return above, this wasn't the attribute we wanted.
            // Skip the data bytes and continue loop.
            currentIndex += dataLength;
        }

        return nullptr; // Attribute not found in this packet
    }
}

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
                if (deviceReading)
                {
                    auto msg = std::make_unique<IncomingMessage>();
                    msg->srcAddress = srcAddr;
                    msg->clusterID = incomingClusterID;
                    msg->deviceReading = std::move(deviceReading);
                    return msg;
                }
            }
        }
        else
        {
            LOG_DEBUG << "[WARNING] AFPacketParser: Unknown Frame Cmd0: " << std::hex << (int)frame.getCommand0()
                      << " Cmd1: " << std::hex << (int)frame.getCommand1() << std::endl;
        }

        return nullptr;
    }
}