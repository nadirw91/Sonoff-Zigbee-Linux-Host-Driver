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

    uint16_t tempShortAddr = 0x90CF;

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
        client.process([&](const ZStackFrame& frame) {            
            // -----------------------------------------------------
            // SCENARIO 1: NEW DEVICE JOINED / ANNOUNCED
            // -----------------------------------------------------
            // if (frame.getCommand0() == (AREQ | ZDO) && 
            //     frame.getCommand1() == ZDO_END_DEVICE_ANNCE_IND) {
                
            //     auto p = frame.getPayload();
            //     uint16_t shortAddr = p[2] | (p[3] << 8);
            //     std::vector<uint8_t> ieeeBytes;
            //     for(int i=0; i<8; i++) ieeeBytes.push_back(p[4+i]);
            //     printIEEE(myIEEE);
            //     std::string ieeeStr = ZStackClient::ieeeToString(ieeeBytes);

            //     // A. Save to Database
            //     deviceDB.addDevice(ieeeStr, shortAddr);

            //     // B. Bind & Configure (Temperature 0x0402)
            //     if (client.bindDevice(shortAddr, ieeeBytes, 0x0402, myIEEE)) {
            //          // Wait a moment for sensor to process Bind before sending Config
            //          std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    
            //          auto afRequest = ZStack::AFDataRequestFactory::configureReporting(shortAddr, 0x0402, 0x29);
                
            //         client.sendAndWait(
            //             afRequest.frame,
            //             afRequest.excpectedResponseCommand0,
            //             afRequest.excpectedResponseCommand1
            //         );
            //     }

            //     // C. Bind & Configure (Humidity 0x0405)
            //     if (client.bindDevice(shortAddr, ieeeBytes, 0x0405, myIEEE)) {
            //          std::this_thread::sleep_for(std::chrono::milliseconds(50));
            //         auto afRequest = ZStack::AFDataRequestFactory::configureReporting(shortAddr, 0x0405, 0x21);
                    
            //         client.sendAndWait(
            //             afRequest.frame,
            //             afRequest.excpectedResponseCommand0,
            //             afRequest.excpectedResponseCommand1
            //         );
            //     }
            // }

            // -----------------------------------------------------
            // SCENARIO 2: INCOMING DATA OR CONFIG RESPONSE
            // -----------------------------------------------------
            // if (frame.getCommand0() == (AREQ | AF) && 
            //          frame.getCommand1() == AF_INCOMING_MSG) {
                
            //     auto p = frame.getPayload();
            //     uint16_t srcAddr = p[4] | (p[5] << 8); 
            //     std::string name = deviceDB.getName(srcAddr);

            //     LOG_DEBUG << ">>> AF_INCOMING_MSG SRC ADDRESS: " << std::hex << std::setw(2) << (int)srcAddr << std::endl; 

            //     LOG_DEBUG << ">>> AF_INCOMING_MSG PAYLOAD SIZE: " << std::hex << std::setw(2) << (int)p.size() << std::endl; 

            //     int dataOffset = 17; // Standard Header Size
            //     if (p.size() <= dataOffset) return; 

            //     uint8_t zclCmd = p[dataOffset + 2];
            //     uint16_t incomingClusterID = p[2] | (p[3] << 8);

            //     LOG_DEBUG << ">>> [Config Response] From " << name 
            //                   << " (Cluster " << getClusterName(incomingClusterID) << ") "
            //                   << "ZCL Cmd: " << getZCLCommandName(zclCmd) << std::endl;

            //     // A. CHECK FOR CONFIG RESPONSE (Receipt)
            //     // ------------------------------------------------
            //     // CASE A: CONFIGURATION RESPONSE (Receipt)
            //     // ------------------------------------------------
            //     if (zclCmd == 0x07) {
            //         uint8_t status = p[dataOffset + 3];
            //         if (status == 0x00) LOG_DEBUG << "    Result: SUCCESS" << std::endl;
            //         else LOG_DEBUG << "    Result: FAIL (Code " << std::hex << (int)status << ")" << std::endl;
            //     }

            //     // ------------------------------------------------
            //     // CASE B: TOGGLE COMMAND (Button Press) - MOVED HERE!
            //     // ------------------------------------------------
            //     else if (incomingClusterID == 0x0006 && zclCmd == 0x02) {
            //         LOG_DEBUG << ">>> [" << name << "] ACTION: Button Pressed (Toggle)" << std::endl;                    
            //     }

            //     // ------------------------------------------------
            //     // CASE C: AUDIT RESPONSE (Read Reporting Config)
            //     // ------------------------------------------------
            //     else if (zclCmd == 0x09) {
            //         // Payload Structure:
            //         // [Status] [Direction] [AttrID_L] [AttrID_H] [DataType] [Min_L] [Min_H] [Max_L] [Max_H] ...
                    
            //         uint8_t status = p[dataOffset + 3];
                    
            //         if (status == 0x00) {
            //             // Success! Let's read the intervals.
            //             // Offsets shift depending on alignment, but standard success is:
            //             // Offset 8,9 = Min Interval
            //             // Offset 10,11 = Max Interval
                        
            //             uint16_t minInt = p[dataOffset + 8] | (p[dataOffset + 9] << 8);
            //             uint16_t maxInt = p[dataOffset + 10] | (p[dataOffset + 11] << 8);
                        
            //             LOG_DEBUG << ">>> [Audit Result] Cluster " << std::hex << incomingClusterID << " Rules:" << std::endl;
            //             LOG_DEBUG << "    Min Interval: " << std::dec << minInt << " seconds" << std::endl;
            //             LOG_DEBUG << "    Max Interval: " << std::dec << maxInt << " seconds" << std::endl;
                        
            //             // Verification Logic
            //             if (maxInt == 600) {
            //                 LOG_DEBUG << "    VERDICT: Config is CORRECT. (Sensor is just ignoring it)" << std::endl;
            //             } else if (maxInt == 65535 || maxInt == 0) {
            //                 LOG_DEBUG << "    VERDICT: Config is MISSING (Factory Default)." << std::endl;
            //             } else {
            //                 LOG_DEBUG << "    VERDICT: Config is set to " << maxInt << " seconds." << std::endl;
            //             }
            //         } else {
            //             LOG_DEBUG << ">>> [Audit Result] Read Failed (Status: " << std::hex << (int)status << ")" << std::endl;
            //         }
            //     }
            //}
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // if (std::chrono::steady_clock::now() - startTime > std::chrono::milliseconds(100) && tempShortAddr != 0) {
        //     startTime = std::chrono::steady_clock::now();
        //     readTemperature(client, tempShortAddr);
        // }
    }
    
    return 0;
}