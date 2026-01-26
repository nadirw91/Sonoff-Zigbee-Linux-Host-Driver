#include <iostream>
#include "ZStackClient.h"
#include "DeviceManager.h"
#include "TemperatureRecorder.h"
#include <thread>
#include <chrono>
#include "AFDataRequest.h"
#include "Logger.h"

using namespace std;
using namespace ZStack; // Use our Namespace

void printIEEE(const std::vector<uint8_t>& ieee) {
    LOG_DEBUG << "IEEE: ";
    for (size_t i = 0; i < ieee.size(); i++) {
        // 1. Set width to 2 chars (so 5 becomes "05")
        // 2. Fill with '0'
        // 3. Convert uint8_t to int so it prints numbers, not weird ASCII symbols
        LOG_DEBUG << std::hex << std::setw(2) << std::setfill('0') 
                  << static_cast<int>(ieee[i]);
        
        // Optional: Add colons between bytes for readability
        if (i < ieee.size() - 1) LOG_DEBUG << ":";
    }
    LOG_DEBUG << std::dec << std::endl; // Reset to decimal just in case
}

string getStateName(uint8_t state) {
    switch(state) {
        case 0x00: return "HOLD (Initialized, not started)";
        case 0x01: return "INIT (Starting...)";
        case 0x02: return "NWK_DISC (Looking for network)";
        case 0x03: return "NWK_JOINING (Joining)";
        case 0x09: return "COORDINATOR (Network Formed & Ready!)";
        default: return "Unknown State";
    }
}

#include "ZStackClient.h"
#include "DeviceManager.h" // <--- Include this

int main() {
    Logger::setLevel(LogLevel::INFO);

    // 1. Setup Database
    DeviceManager deviceDB("devices.txt");
    TemperatureRecorder tempRecorder("temperature_readings.txt");

    // 2. Connect to Hardware
    ZStackClient client("/dev/ttyUSB0");
    if (!client.connect()) {
        std::cerr << "Failed to connect to Serial Port" << std::endl;
        return -1;
    }

    // 3. Initialize Zigbee Stack
    client.reset();
    client.registerEndpoint();
    client.startNetwork();
    std::vector<uint8_t> myIEEE = client.getDeviceState()->iEEE_address;
    printIEEE(myIEEE);
    
    // 4. Open Network for Joining
    client.permitJoin(60); 

    LOG_INFO << "--- Main Loop Started ---" << std::endl;
    auto startTime = std::chrono::steady_clock::now();
    auto timeout = 100;


    client.setZdoPacketHandler([&](const ZDOPacket::Packet& packet) {
        if (packet.type == ZDOPacket::DEVICE_ANNOUNCEMENT) {
            auto devAnnce = static_cast<const ZDOPacket::DeviceAnnouncementResponse&>(packet);
            LOG_INFO << ">>> [ZDO] Device Announcement Received: ";
            LOG_INFO << "ShortAddr=" << std::hex << devAnnce.srcAddress;
            LOG_INFO << " IEEE=" << std::hex << devAnnce.ieeeAddress;
            LOG_INFO << " Type: " << devAnnce.type << "\n";

            client.fetchActiveEndpoints(devAnnce.srcAddress);
            // Get Device Capabilities
        } else if (packet.type == ZDOPacket::ACTIVE_ENDPOINTS) {
            auto activeEp = static_cast<const ZDOPacket::DeviceActiveEndpointResponse&>(packet);
            LOG_INFO << ">>> [ZDO] Active Endpoints for ShortAddr=" 
                      << std::hex << activeEp.srcAddress 
                      << ": ";

            printIEEE(myIEEE);
            if (activeEp.activeEndpoints.size() > 0) {
                for (auto ep : activeEp.activeEndpoints) {
                    LOG_INFO << std::hex << (int)ep << " ";
                }
                LOG_INFO << std::dec << std::endl;  
                client.fetchSimpleDescriptor(activeEp.srcAddress, activeEp.activeEndpoints[0]);
            } else {
                LOG_INFO << "No Active Endpoints Found" << std::dec << std::endl;
            }            

        } else if (packet.type == ZDOPacket::DEVICE_DESCRIPTION) {
            auto simpleDesc = static_cast<const ZDOPacket::DeviceDescriptionResponse&>(packet);
            LOG_INFO << ">>> [ZDO] Simple Descriptor for ShortAddr=" 
                      << std::hex << simpleDesc.sourceAddress 
                      << " Endpoint=" << std::dec << (int)simpleDesc.endpoint 
                      << ": InClusters=[";
            for (auto cid : simpleDesc.inputClusters) {
                LOG_INFO << std::hex << cid << " ";
            }
            LOG_INFO << "] OutClusters=[";
            for (auto cid : simpleDesc.outputClusters) {
                LOG_INFO << std::hex << cid << " ";
            }
            LOG_INFO << "]" << std::dec << std::endl;
        } else if (packet.type == ZDOPacket::BIND_RESPONSE) {
            auto bindResp = static_cast<const ZDOPacket::BindRequestResponse&>(packet);
            LOG_INFO << ">>> [ZDO] Bind Response from ShortAddr=" 
                      << std::hex << bindResp.srcAddress 
                      << ": " << (bindResp.success ? "SUCCESS" : "FAILURE") << std::dec << std::endl;
        }
    });

    client.setAfPacketHandler([&](const AFPacket::Packet& packet) {
        LOG_DEBUG << ">>> [AF] Incoming Message from " << std::hex << (int) packet.type << std::endl;
        
        if (packet.type == AFPacket::AF_INCOMING_MSG) {
            auto& incomingMsg = static_cast<const AFPacket::IncomingMessage&>(packet);
            LOG_DEBUG << ">>> [AF] Incoming Message from " 
                      << std::hex << (int) incomingMsg.srcAddress 
                      << " (Cluster " << std::hex << (int) incomingMsg.clusterID << ")" << std::endl;
            // Save to Temperature Recorder
            if (incomingMsg.deviceReading->type == AFPacket::TEMPERATURE_SENSOR) {
                auto& tempReading = static_cast<const AFPacket::TemperatureReading&>(*incomingMsg.deviceReading);
                LOG_DEBUG << "    Temperature: " << std::fixed << std::setprecision(2) 
                          << tempReading.temperatureReading << " C" << std::endl;
            
                tempRecorder.saveTemperatureReading(tempReading.temperatureReading);
            }
        }
    });

    while(true) {
        client.process();

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    }
    
    return 0;
}