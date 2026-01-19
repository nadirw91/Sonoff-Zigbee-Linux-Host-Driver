#ifndef AF_DATA_REQUEST_H
#define AF_DATA_REQUEST_H

#include <vector>
#include "ZStackFrame.h"
#include <iostream>

namespace ZStack
{
    struct AFDataRequest
    {
        ZStackFrame frame;
        uint8_t excpectedResponseCommand0;
        uint8_t excpectedResponseCommand1;
    };

    class AFDataRequestFactory
    {
    public:
        static AFDataRequest readTemperature(uint16_t shortAddr)
        {
            std::cout << "[Command] Asking device " << std::hex << shortAddr << " for Temperature..." << std::endl;

            // Payload for "Read Attributes" (Command 0x00)
            std::vector<uint8_t> payload;
            payload.push_back(0x00); // Frame Control
            payload.push_back(0x01); // Sequence
            payload.push_back(0x00); // Command: Read Attributes
            payload.push_back(0x00); // Attr ID Low (Measured Value)
            payload.push_back(0x00); // Attr ID High

            // Wrap in AF_DATA_REQUEST
            std::vector<uint8_t> afPayload;
            afPayload.push_back(shortAddr & 0xFF);
            afPayload.push_back((shortAddr >> 8) & 0xFF);
            afPayload.push_back(0x01); // Dst Endpoint
            afPayload.push_back(0x01); // Src Endpoint
            afPayload.push_back(0x02); // Cluster Low (0x0402)
            afPayload.push_back(0x04); // Cluster High
            afPayload.push_back(0x00); // TransID
            afPayload.push_back(0x00); // Options
            afPayload.push_back(0x0F); // Radius
            afPayload.push_back(payload.size());
            afPayload.insert(afPayload.end(), payload.begin(), payload.end());

            ZStackFrame req(SREQ | AF, AF_DATA_REQUEST, afPayload);

            AFDataRequest afRequest;
            afRequest.frame = req;
            afRequest.excpectedResponseCommand0 = SRSP | AF;
            afRequest.excpectedResponseCommand1 = AF_DATA_REQUEST;

            return afRequest;
        };

        static AFDataRequest readHumidity(uint16_t shortAddr)
        {
            std::cout << "[Command] Asking device " << std::hex << shortAddr << " for Humidity..." << std::endl;

            std::vector<uint8_t> payload;

            payload.push_back(0x00); // Frame Control
            payload.push_back(0x01); // Sequence
            payload.push_back(0x00); // Command: Read Attributes
            payload.push_back(0x00); // Attr ID Low (Measured Value)
            payload.push_back(0x00); // Attr ID High

            // Wrap in AF_DATA_REQUEST
            std::vector<uint8_t> afPayload;
            afPayload.push_back(shortAddr & 0xFF);
            afPayload.push_back((shortAddr >> 8) & 0xFF);
            afPayload.push_back(0x01); // Dst Endpoint
            afPayload.push_back(0x01); // Src Endpoint
            afPayload.push_back(0x05); // Cluster Low (0x0402)
            afPayload.push_back(0x04); // Cluster High
            afPayload.push_back(0x00); // TransID
            afPayload.push_back(0x00); // Options
            afPayload.push_back(0x0F); // Radius
            afPayload.push_back(payload.size());
            afPayload.insert(afPayload.end(), payload.begin(), payload.end());

            ZStackFrame req(SREQ | AF, AF_DATA_REQUEST, afPayload);

            AFDataRequest afRequest;
            afRequest.frame = req;
            afRequest.excpectedResponseCommand0 = SRSP | AF;
            afRequest.excpectedResponseCommand1 = AF_DATA_REQUEST;

            return afRequest;
        };

        static AFDataRequest configureReporting(uint16_t shortAddr, uint16_t clusterID, uint8_t dataType)
        {
            std::cout << "[Config] Sending Reporting Configuration to " << std::hex << shortAddr
                      << " for Cluster " << clusterID << "..." << std::endl;

            std::vector<uint8_t> payload;
            payload.push_back(0x00); // Frame Control
            payload.push_back(0x11); // Sequence
            payload.push_back(0x06); // Command: Configure Reporting

            // Payload Details
            payload.push_back(0x00);     // Direction: Reported
            payload.push_back(0x00);     // Attr ID Low (Measured Value)
            payload.push_back(0x00);     // Attr ID High
            payload.push_back(dataType); // Data Type: INT16 (Temp/Humidity standard)

            // Min Interval: 10 seconds (0x000A)
            payload.push_back(0x0A);
            payload.push_back(0x00);

            // Max Interval: 10 Minutes (600s = 0x0258)
            // Little Endian -> 58 02
            payload.push_back(0x58);
            payload.push_back(0x02);

            // Reportable Change: 0.20 (Value 20 = 0x0014)
            payload.push_back(0x14);
            payload.push_back(0x00);

            // Wrap in AF_DATA_REQUEST
            std::vector<uint8_t> afPayload;
            afPayload.push_back(shortAddr & 0xFF);
            afPayload.push_back((shortAddr >> 8) & 0xFF);
            afPayload.push_back(0x01);                    // Dst Endpoint
            afPayload.push_back(0x01);                    // Src Endpoint
            afPayload.push_back(clusterID & 0xFF);        // Cluster Low
            afPayload.push_back((clusterID >> 8) & 0xFF); // Cluster High
            afPayload.push_back(0x00);                    // TransID
            afPayload.push_back(0x00);                    // Options
            afPayload.push_back(0x0F);                    // Radius
            afPayload.push_back(payload.size());
            afPayload.insert(afPayload.end(), payload.begin(), payload.end());

            ZStackFrame req(SREQ | AF, AF_DATA_REQUEST, afPayload);

            AFDataRequest afRequest;
            afRequest.frame = req;
            afRequest.excpectedResponseCommand0 = SRSP | AF;
            afRequest.excpectedResponseCommand1 = AF_DATA_REQUEST;

            return afRequest;
        };

        static AFDataRequest readReportingConfig(uint16_t shortAddr, uint16_t clusterID)
        {
            std::cout << "[Audit] Asking device " << std::hex << shortAddr
                      << " for Reporting Config (Cluster " << clusterID << ")..." << std::endl;

            std::vector<uint8_t> payload;
            payload.push_back(0x00); // Frame Control
            payload.push_back(0x12); // Sequence Number
            payload.push_back(0x08); // Command: Read Reporting Configuration

            // Payload: Which attribute do we want to check? (0x0000 Measured Value)
            payload.push_back(0x00); // Direction: Reported
            payload.push_back(0x00); // Attr ID Low
            payload.push_back(0x00); // Attr ID High

            // Wrap in AF_DATA_REQUEST
            std::vector<uint8_t> afPayload;
            afPayload.push_back(shortAddr & 0xFF);
            afPayload.push_back((shortAddr >> 8) & 0xFF);
            afPayload.push_back(0x01);                    // Dst Endpoint
            afPayload.push_back(0x01);                    // Src Endpoint
            afPayload.push_back(clusterID & 0xFF);        // Cluster Low
            afPayload.push_back((clusterID >> 8) & 0xFF); // Cluster High
            afPayload.push_back(0x00);                    // TransID
            afPayload.push_back(0x00);                    // Options
            afPayload.push_back(0x0F);                    // Radius
            afPayload.push_back(payload.size());
            afPayload.insert(afPayload.end(), payload.begin(), payload.end());

            ZStackFrame req(SREQ | AF, AF_DATA_REQUEST, afPayload);

            AFDataRequest afRequest;
            afRequest.frame = req;
            afRequest.excpectedResponseCommand0 = SRSP | AF;
            afRequest.excpectedResponseCommand1 = AF_DATA_REQUEST;

            return afRequest;
        };
    };
};

#endif