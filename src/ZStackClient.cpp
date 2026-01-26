#include "ZStackClient.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include "zdo/ZDOPacketParser.h"
#include "af/AFPacketParser.h"
#include "Logger.h"

namespace ZStack
{
    ZStackClient::ZStackClient(const std::string &portName)
    {
        serialPort = std::make_unique<SerialPort>(portName);
        zdoPacketHandler = nullptr;
        afPacketHandler = nullptr;
    }

    ZStackClient::~ZStackClient()
    {
        close();
    }

    bool ZStackClient::connect()
    {
        return serialPort->openPort();
    }

    void ZStackClient::close()
    {
        serialPort->closePort();
    }

    std::optional<ZStackFrame> ZStackClient::waitForFrame(uint8_t expectedCmd0,
                                                          uint8_t expectedCmd1,
                                                          int timeoutMs)
    {
        auto startTime = std::chrono::steady_clock::now();
        std::vector<uint8_t> buffer;

        while (std::chrono::steady_clock::now() - startTime < std::chrono::milliseconds(timeoutMs))
        {
            int bytes = serialPort->readBytes(buffer);

            if (bytes > 0)
            {
                for (uint8_t byte : buffer)
                {
                    // Parse every byte
                    auto result = parser.parseByte(byte);

                    if (result.has_value())
                    {
                        ZStackFrame frame = result.value();

                        // DEBUG: Print what we found so we aren't flying blind
                        LOG_DEBUG << "[DEBUG] Rx: " << "Length: " << std::hex << std::setw(2) << (int)frame.getPayload().size() << " Cmd0: " << (int)frame.getCommand0()
                                  << " Cmd1: " << (int)frame.getCommand1() << std::endl;

                        if (frame.getCommand0() == expectedCmd0 &&
                            frame.getCommand1() == expectedCmd1)
                        {
                            return frame;
                        }
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return std::nullopt; // Timeout
    }

    std::optional<ZStackFrame> ZStackClient::sendAndWait(
        const ZStackFrame &request,
        uint8_t expectedCmd0,
        uint8_t expectedCmd1,
        int timeoutMs)
    {
        serialPort->writeBytes(request.toSerialBytes());

        // Wait
        return waitForFrame(expectedCmd0, expectedCmd1, timeoutMs);
    }

    std::optional<SysVersion> ZStackClient::getSystemVersion(int timeoutMs)
    {
        LOG_DEBUG << "Getting System Version..." << std::endl;

        ZStackFrame req(SREQ | SYS, SYS_VERSION);

        auto response = sendAndWait(req, SRSP | SYS, SYS_VERSION, timeoutMs);

        if (!response)
        {
            std::cerr << "[ERROR] No response for SYS_VERSION request." << std::endl;
            return std::nullopt;
        }

        const auto &payload = response->getPayload();

        if (payload.size() < 5)
            return std::nullopt;

        SysVersion version;
        version.transport = payload[0];
        version.product = payload[1];
        version.major = payload[2];
        version.minor = payload[3];
        version.maint = payload[4];

        if (payload.size() >= 9)
        {
            version.revision = static_cast<uint32_t>(payload[5]) |
                               (static_cast<uint32_t>(payload[6]) << 8) |
                               (static_cast<uint32_t>(payload[7]) << 16) |
                               (static_cast<uint32_t>(payload[8]) << 24);
        }

        return version;
    }

    bool ZStackClient::startNetwork()
    {
        // 1. Give the bus a moment to breathe after the previous command
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 2. Construct the Payload (Start Delay = 100ms)
        std::vector<uint8_t> payload = {0x64, 0x00};
        ZStackFrame req(SREQ | ZDO, ZDO_STARTUP_FROM_APP, payload);

        LOG_DEBUG << "Starting Network..." << std::endl; // <--- This was missing in your logs!

        bool commandAccepted = false;
        for (int attempt = 1; attempt <= 3; ++attempt)
        {
            LOG_DEBUG << "Attempt " << attempt << " to start network." << std::endl;

            // 3. Send Command & Wait for ACK
            auto ack = sendAndWait(req, SRSP | ZDO, ZDO_STARTUP_FROM_APP, 3000);

            if (ack)
            {
                LOG_DEBUG << "ACK received." << std::endl;
                commandAccepted = true;
                break;
            }
            else
            {
                LOG_DEBUG << "No ACK received, retrying..." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }

        // Check Status inside ACK (0 = Success)
        if (!commandAccepted)
        {
            LOG_DEBUG << "Failed to accept start network command" << std::endl;
            return false;
        }

        LOG_DEBUG << "Waiting for State Change..." << std::endl;

        // 3. Try waiting for the Event (Fast path)
        auto stateMsg = waitForFrame(AREQ | ZDO, ZDO_STATE_CHANGE_IND, 5000);

        if (stateMsg)
        {
            uint8_t state = stateMsg->getPayload()[0];
            if (state == 0x09)
            {
                LOG_DEBUG << "Network Started! (Event: Coordinator)" << std::endl;
                return true;
            }
        }

        // 4. FALLBACK: Event lost? Poll status directly.
        LOG_DEBUG << "Event timeout. Polling device state..." << std::endl;
        auto state = getDeviceState();

        if (state)
        {
            LOG_DEBUG << "Polled State: 0x" << std::hex << state->state << std::endl;
            if (state->state == 0x09)
            {
                LOG_DEBUG << "Network Started! (Polled: Coordinator)" << std::endl;
                return true;
            }
        }

        return false;
    }

    std::optional<DeviceState> ZStackClient::getDeviceState()
    {
        LOG_DEBUG << "Getting Device State..." << std::endl;

        // Command: UTIL_GET_DEVICE_INFO (0x27 0x00)
        // This asks the dongle: "Tell me everything about your state"
        ZStackFrame req(SREQ | UTIL, UTIL_GET_DEVICE_INFO);

        auto resp = sendAndWait(req, SRSP | UTIL, UTIL_GET_DEVICE_INFO);

        auto payload = resp->getPayload();

        if (resp && resp->getPayload().size() > 8)
        {
            DeviceState state;

            std::vector<uint8_t> ieee_addr;

            for (int i = 1; i <= 8; i++)
            {
                ieee_addr.push_back(payload[i]);
            }

            state.iEEE_address = ieee_addr;

            state.short_address = payload[9] | (static_cast<uint16_t>(payload[10]) << 8);

            state.device_type = payload[11];

            state.state = payload[12];

            LOG_DEBUG << "Device State Info:" << std::endl;
            LOG_DEBUG << "  IEEE Address: " << std::endl;
            for (int i = 7; i >= 0; i--)
                printf(" %02X", state.iEEE_address[i]);
            LOG_DEBUG << "  Short Address: 0x" << std::hex << std::setw(4) << std::setfill('0') << state.short_address << std::endl;
            LOG_DEBUG << "  Device Type: 0x" << std::hex << state.device_type << std::endl;
            LOG_DEBUG << "  State: 0x" << std::hex << state.state << std::endl;

            return state;
        }

        return std::nullopt;
    }

    // Add to your ZStackClient class
    bool ZStackClient::permitJoin(uint8_t durationSeconds)
    {
        LOG_DEBUG << "Permitting Join for " << (int)durationSeconds << " seconds..." << std::endl;

        std::vector<uint8_t> payload;

        // 1. AddrMode (From your screenshot: 0x02 = Address 16 bit)
        // We are telling the dongle: "The next 2 bytes are a 16-bit address"
        payload.push_back(0x02);

        // 1. Destination Address (0xFFFC = All Routers & Coordinator)
        // Little Endian: FC FF
        payload.push_back(0xFC);
        payload.push_back(0xFF);

        // 2. Duration (0 = Off, 255 = Always On, 1-254 = Seconds)
        payload.push_back(durationSeconds);

        // 3. Trust Center Significance (Always 1 or 0, usually 0 works fine)
        payload.push_back(0x00);

        // Build Frame
        ZStackFrame req(SREQ | ZDO, ZDO_MGMT_PERMIT_JOIN_REQ, payload);

        // Send and wait for success (0x00)
        auto ack = sendAndWait(req, SRSP | ZDO, ZDO_MGMT_PERMIT_JOIN_REQ);

        if (ack && ack->getPayload().size() > 0 && ack->getPayload()[0] == 0)
        {
            LOG_DEBUG << "Join Enabled! Devices can pair now." << std::endl;
            return true;
        }

        LOG_DEBUG << "Failed to enable joining." << std::endl;
        return false;
    }

    void ZStackClient::process()
    {
        std::vector<uint8_t> buffer;

        // 1. Read available bytes (Non-blocking)
        int bytes = serialPort->readBytes(buffer);

        if (bytes > 0)
        {
            for (uint8_t byte : buffer)
            {
                auto result = parser.parseByte(byte);

                // 2. Did we finish a packet?
                if (result.has_value())
                {
                    ZStackFrame frame = result.value();

                    routeFrameToParser(frame);

                    LOG_DEBUG << ">>> DEBUG PROCESS FRAME FOUND" << std::endl;
                }
            }
        }
    }

    bool ZStackClient::registerEndpoint()
    {
        LOG_DEBUG << "Registering Endpoint..." << std::endl;

        std::vector<uint8_t> payload;

        // 1. Endpoint id (1-240)
        payload.push_back(0x01); // Endpoint 1

        // 2. Application Profile ID
        // 0x0104 = Home Automation Profile (Public profile, Little Endian: 04 01)
        payload.push_back(0x04);
        payload.push_back(0x01);

        // 3. Application Device ID (2 Bytes)
        // 0x0007 = Configuration Tool / Controller (Little Endian: 07 00)
        payload.push_back(0x07);
        payload.push_back(0x00);

        // 4. Application Device Version (1 Byte)
        payload.push_back(0x00);

        // 5. Latency Requirements (0 = No Latency)
        payload.push_back(0x00);

        // 6. Input Clusters (What we listen for)
        // Let's say we listen for nothing for now to keep it simple
        payload.push_back(0x00); // Count = 0

        // 7 Output Clusters (What we control)
        payload.push_back(0x02); // Count = 2

        // Temperature (0x0402);
        payload.push_back(0x02);
        payload.push_back(0x04);

        // Humidity (0x0405);
        payload.push_back(0x05);
        payload.push_back(0x04);

        ZStackFrame req(SREQ | AF, AF_REGISTER, payload);

        auto ack = sendAndWait(req, SRSP | AF, AF_REGISTER);

        bool payloadValid = ack.has_value();
        bool payloadSizeValid = (ack && ack->getPayload().size() > 0);

        LOG_DEBUG << "Registration Payload Valid: " << payloadValid << std::endl;
        LOG_DEBUG << "Registration Payload Size Valid: " << payloadSizeValid << std::endl;

        if (payloadSizeValid && ack->getPayload()[0] == 0)
        {
            LOG_DEBUG << "Endpoint Registered Successfully!" << std::endl;
            return true;
        }
        else
        {
            LOG_DEBUG << "Failed to register endpoint." << std::endl;
            return false;
        }
    }

    void ZStackClient::reset()
    {
        LOG_DEBUG << "Resetting Dongle..." << std::endl;

        // Command: SYS_RESET_REQ (0x41 0x00)
        // Payload: 0x01 (Soft Reset)
        std::vector<uint8_t> payload = {0x01};

        // Note: We use AREQ (Async Request) because we don't expect a polite synchronous reply.
        // The device will just reboot.
        ZStackFrame resetCmd(AREQ | SYS, SYS_RESET_REQ, payload);

        serialPort->writeBytes(resetCmd.toSerialBytes());

        auto confirmation = waitForFrame(AREQ | SYS, 0x80, 5000);

        if (confirmation)
        {
            LOG_DEBUG << "Dongle Reset Confirmed." << std::endl;
        }
        else
        {
            LOG_DEBUG << "No confirmation received, but proceeding anyway." << std::endl;
        }

        LOG_DEBUG << "Dongle Reset Complete." << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    bool ZStackClient::bindDevice(
        uint16_t targetShortAddr,
        const std::vector<uint8_t> &targetIEEE, // The Sensor's IEEE
        uint16_t clusterID,
        const std::vector<uint8_t> &myIEEE // Your Coordinator's IEEE
    )
    {
        LOG_DEBUG << "Binding Cluster 0x" << std::hex << clusterID << "..." << std::endl;

        std::vector<uint8_t> payload;

        // 1. Destination (Short Address of the device we are configuring)
        // Splitting 16 bit int into 2 8-bit ints (Little Endian)
        payload.push_back(targetShortAddr & 0xFF);
        payload.push_back((targetShortAddr >> 8) & 0xFF);

        // 2. Source IEEE (The Sensor's Long Address) - 8 Bytes
        // MUST be sent in Little Endian (Reverse of what you see in logs usually)
        // Assuming the vector passed in is already correct (from the Announce packet)
        payload.insert(payload.end(), targetIEEE.begin(), targetIEEE.end());

        // 3. Source Endpoint (The Sensor's "Port")
        // Most sensors transmit from Endpoint 1.
        payload.push_back(0x01);

        // 4. Cluster ID (What data to send? Temp = 0x0402)
        payload.push_back(clusterID & 0xFF);
        payload.push_back((clusterID >> 8) & 0xFF);

        // 5. Destination Address Mode (3 = 64-bit IEEE Address)
        payload.push_back(0x03);

        // 6. Destination IEEE (Who receives the data? US!)
        payload.insert(payload.end(), myIEEE.begin(), myIEEE.end());

        // 7. Destination Endpoint (Our "Port" - we registered Endpoint 1 earlier)
        payload.push_back(0x01);

        // Send Request
        ZStackFrame req(SREQ | ZDO, ZDO_BIND_REQ, payload);
        auto ack = sendAndWait(req, SRSP | ZDO, ZDO_BIND_REQ);

        if (ack && ack->getPayload().size() > 0 && ack->getPayload()[0] == 0)
        {
            LOG_DEBUG << "Bind Success for Cluster 0x" << std::hex << clusterID << "!" << std::endl;
            return true;
        }

        LOG_DEBUG << "Bind Failed." << std::endl;

        return false;
    }

    void ZStackClient::fetchActiveEndpoints(
        uint16_t targetShortAddr
    ) {
        LOG_DEBUG << "Fetching Active Endpoints for 0x" << std::hex << targetShortAddr << "..." << std::endl;

        std::vector<uint8_t> payload;

        payload.push_back(targetShortAddr & 0xFF);
        payload.push_back((targetShortAddr >> 8) & 0xFF);

        payload.push_back(targetShortAddr & 0xFF);
        payload.push_back((targetShortAddr >> 8) & 0xFF);

        ZStackFrame req(SREQ | ZDO, ZDO_ACTIVE_EP_REQ, payload);

        auto ack = serialPort->writeBytes(req.toSerialBytes());
    }

    void ZStackClient::fetchSimpleDescriptor(
        uint16_t targetShortAddr,
        uint8_t endpoint
    ) {
        LOG_DEBUG << "Fetching Simple Descriptor for 0x" << std::hex << targetShortAddr 
                  << " Endpoint " << std::dec << (int)endpoint << "..." << std::endl;

        std::vector<uint8_t> payload;

        // 2. NWK Address of Interest (Short Address again)
        payload.push_back(targetShortAddr & 0xFF);
        payload.push_back((targetShortAddr >> 8) & 0xFF);

        // 1. Target Address (Short Address of the device we are querying)
        payload.push_back(targetShortAddr & 0xFF);
        payload.push_back((targetShortAddr >> 8) & 0xFF);

        // 2. Endpoint
        payload.push_back(endpoint);

        ZStackFrame req(SREQ | ZDO, ZDO_SIMPLE_DESC_REQ, payload);

        auto ack = serialPort->writeBytes(req.toSerialBytes());
    }

    void ZStackClient::routeFrameToParser(const ZStackFrame &frame)
    {
        LOG_DEBUG << "Routing Frame to Parser:" << std::endl;

        auto command0 = frame.getCommand0();

        auto subsystem = command0 & 0x1F;

        switch (subsystem)
        {
        case AF:
        {
            // LOG_DEBUG << "AF Frame Detected:" << std::hex << std::setw(2) << (int)subsystem << std::endl;
            // auto afResponse = AFPacket::parseZStackFrame(frame);

            // if (afResponse == nullptr)
            // {
            //     LOG_DEBUG << "Client AF Packet Parser returned null." << std::endl;
            // }
            // else
            // {
            //     LOG_DEBUG << "Client AF Packet Parser returned valid packet. Type: " << (int)afResponse->type << std::endl;
            // }

            // if (afResponse && afPacketHandler)
            // {
            //     afPacketHandler(*afResponse);
            // }
            break;
        }
        case ZDO:
        {
            LOG_DEBUG << "ZDO Frame Detected:" << std::hex << std::setw(2) << (int)subsystem << std::endl;
            auto zdoResponse = ZDOPacket::parseZStackFrame(frame);
            if (zdoResponse && zdoPacketHandler)
            {
                zdoPacketHandler(*zdoResponse);
            }
            else
            {
                LOG_DEBUG << "No ZDO Packet Handler Registered." << std::endl;
            }
            break;
        }
        default:
        {
            LOG_DEBUG << "[WARNING] Unknown/Unhandled Subsystem: " << (int)subsystem << std::endl;
            break;
        }
        }
    }
}