#ifndef ZSTACK_PROTOCOL_H
#define ZSTACK_PROTOCOL_H

#include <cstdint>
#include <map>
#include <string>
#include <sstream>
#include <iomanip>

namespace ZStack
{

    // --- 1. Command Types (Top 3 bits) ---
    // Usage: (Type | Subsystem)
    enum CommandType : uint8_t
    {
        POLL = 0x00, // Polling (Rare)
        SREQ = 0x20, // Synchronous Request (We ask, it answers immediately)
        AREQ = 0x40, // Asynchronous Request (Fire and forget)
        SRSP = 0x60  // Synchronous Response (The answer we get back)
    };

    // --- 2. Subsystems (Bottom 5 bits) ---
    enum Subsystem : uint8_t
    {
        SYS = 0x01,  // System Interface (Version, Reset, Ping)
        MAC = 0x02,  // MAC Layer (Low level radio config)
        NWK = 0x03,  // Network Layer
        AF = 0x04,   // Application Framework (Sending data to bulbs!)
        ZDO = 0x05,  // Zigbee Device Object (Pairing, Discovery)
        SAPI = 0x06, // Simple API
        UTIL = 0x07  // Utilities
    };

    // --- 3. Specific Command IDs (CMD1) ---

    // SYS Subsystem Commands
    enum SysCommandID : uint8_t
    {
        SYS_RESET_REQ = 0x00,
        SYS_PING = 0x01,
        SYS_VERSION = 0x02,
        SYS_SET_EXTADDR = 0x03,
        SYS_GET_EXTADDR = 0x0D
    };

    // AF Subsystem Commands (You will use these later)
    enum AFCommandID : uint8_t
    {
        AF_REGISTER = 0x00,
        AF_DATA_REQUEST = 0x01, // The "Send Message" command
        AF_INCOMING_MSG = 0x81  // (Incoming) Message Received
    };

    enum ZDOCommandID : uint8_t
    {
        ZDO_STARTUP_FROM_APP = 0x40,     // Start the network
        ZDO_STATE_CHANGE_IND = 0xC0,     // (Incoming) The chip tells us its status changed
        ZDO_MGMT_PERMIT_JOIN_REQ = 0x36, // Allow other devices to join
        ZDO_END_DEVICE_ANNCE_IND = 0xC1, // (Incoming) A new device has joined

        ZDO_BIND_REQ = 0x21, // Create a binding
        ZDO_BIND_RSP = 0xA1,

        ZDO_ACTIVE_EP_REQ = 0x05, // Request Simple Descriptor
        ZDO_ACTIVE_EP_RSP = 0x85, // Response Active Endpoints

        ZDO_SIMPLE_DESC_REQ = 0x04, // Request Simple Descriptor
        ZDO_SIMPLE_DESC_RSP = 0x84
    };

    enum UtilCommandID : uint8_t
    {
        UTIL_GET_DEVICE_INFO = 0x00
    };

    enum ZCLCommandID : uint8_t
    {
        ZCL_READ_ATTRIB_REQ = 0x00,
        ZCL_READ_ATTRIB_RSP = 0x01,
        ZCL_WRITE_ATTRIB_REQ = 0x02,
        ZCL_WRITE_ATTRIB_RSP = 0x03,
        ZCL_CONFIG_REPORTING_REQ = 0x06,
        ZCL_CONFIG_REPORTING_RSP = 0x07,
        ZCL_REPORT_ATTRIB = 0x0A,
        ZCL_DEFAULT_RSP = 0x0B,
        ZCL_DISCOVER_ATTRIBS_REQ = 0x0C,
        ZCL_DISCOVER_ATTRIBS_RSP = 0x0D
    };

    enum ClusterID : uint16_t
    {
        ON_OFF_CLUSTER = 0x0006,
        LEVEL_CONTROL_CLUSTER = 0x0008,
        COLOR_CONTROL_CLUSTER = 0x0300,
        TEMPERATURE_MEASUREMENT_CLUSTER = 0x0402,
        HUMIDITY_MEASUREMENT_CLUSTER = 0x0405,
        BATTERY_LEVEL_CLUSTER = 0x0001,

        // WARN: In ZCL, 0x0702 is usually Summation (Consumption)
        // and 0x0B04 is Electrical Msmt (Instant).
        // Your enum names are swapped relative to the hex codes.
        INSTANTANEOUS_POWER_CONSUMPTION_CLUSTER = 0x0702,
        POWER_CONSUMPTION_CLUSTER = 0x0B04
    };

    const std::map<uint8_t, std::string> zclCommandNameMap =
        {
            {ZCL_READ_ATTRIB_REQ, "ZCL_READ_ATTRIB_REQ"},
            {ZCL_READ_ATTRIB_RSP, "ZCL_READ_ATTRIB_RSP"},
            {ZCL_WRITE_ATTRIB_REQ, "ZCL_WRITE_ATTRIB_REQ"},
            {ZCL_WRITE_ATTRIB_RSP, "ZCL_WRITE_ATTRIB_RSP"},
            {ZCL_CONFIG_REPORTING_REQ, "ZCL_CONFIG_REPORTING_REQ"},
            {ZCL_CONFIG_REPORTING_RSP, "ZCL_CONFIG_REPORTING_RSP"},
            {ZCL_REPORT_ATTRIB, "ZCL_REPORT_ATTRIB"},
            {ZCL_DEFAULT_RSP, "ZCL_DEFAULT_RSP"},
            {ZCL_DISCOVER_ATTRIBS_REQ, "ZCL_DISCOVER_ATTRIBS_REQ"},
            {ZCL_DISCOVER_ATTRIBS_RSP, "ZCL_DISCOVER_ATTRIBS_RSP"}};

    const std::map<uint16_t, std::string> clusterNameMap =
        {
            {ON_OFF_CLUSTER, "On/Off Cluster"},
            {LEVEL_CONTROL_CLUSTER, "Level Control Cluster"},
            {COLOR_CONTROL_CLUSTER, "Color Control Cluster"},
            {TEMPERATURE_MEASUREMENT_CLUSTER, "Temperature Measurement Cluster"},
            {HUMIDITY_MEASUREMENT_CLUSTER, "Humidity Measurement Cluster"},
            {BATTERY_LEVEL_CLUSTER, "Battery Level Cluster"}};

    const std::map<uint8_t, std::string> commandNameMap = {
        // SYS Commands
        {SYS_RESET_REQ, "SYS_RESET_REQ"},
        {SYS_PING, "SYS_PING"},
        {SYS_VERSION, "SYS_VERSION"},
        {SYS_SET_EXTADDR, "SYS_SET_EXTADDR"},
        {SYS_GET_EXTADDR, "SYS_GET_EXTADDR"},

        // AF Commands
        {AF_REGISTER, "AF_REGISTER"},
        {AF_DATA_REQUEST, "AF_DATA_REQUEST"},
        {AF_INCOMING_MSG, "AF_INCOMING_MSG"},

        // ZDO Commands
        {ZDO_STARTUP_FROM_APP, "ZDO_STARTUP_FROM_APP"},
        {ZDO_STATE_CHANGE_IND, "ZDO_STATE_CHANGE_IND"},
        {ZDO_MGMT_PERMIT_JOIN_REQ, "ZDO_MGMT_PERMIT_JOIN_REQ"},
        {ZDO_END_DEVICE_ANNCE_IND, "ZDO_END_DEVICE_ANNCE_IND"},
        {ZDO_BIND_REQ, "ZDO_BIND_REQ"},

        // UTIL Commands
        {UTIL_GET_DEVICE_INFO, "UTIL_GET_DEVICE_INFO"}};

    inline std::string toHex(int value, int width = 2)
    {
        std::stringstream ss;
        ss << std::hex << std::uppercase << std::setfill('0') << std::setw(width) << value;
        return "0x" + ss.str();
    }

    inline std::string getCommandName(uint8_t cmd0, uint8_t cmd1)
    {
        // 1. SYSTEM COMMANDS (SYS - 0x41/0x21)
        // Check Subsystem Mask (0x1F) == 1 (SYS)
        if ((cmd0 & 0x1F) == 0x01)
        {
            if (cmd1 == 0x80)
                return "SYS_RESET_IND";
            if (cmd1 == 0x00)
                return "SYS_PING";
            if (cmd1 == 0x01)
                return "SYS_VERSION";
        }

        // 2. AF COMMANDS (AF - 0x44/0x24)
        // Check Subsystem Mask (0x1F) == 4 (AF)
        if ((cmd0 & 0x1F) == 0x04)
        {
            switch (cmd1)
            {
            case 0x00:
                return "AF_REGISTER";
            case 0x01:
                return "AF_DATA_REQUEST";
            case 0x80:
                return "AF_DATA_CONFIRM"; // <--- This is what you were seeing!
            case 0x81:
                return "AF_INCOMING_MSG";
            }
        }

        // 3. ZDO COMMANDS (Zigbee Device Object - 0x25/0x45/0x65)
        if ((cmd0 & 0x1F) == 0x05)
        {
            switch (cmd1)
            {
            case 0x00:
                return "ZDO_NWK_ADDR_REQ";
            case 0x01:
                return "ZDO_IEEE_ADDR_REQ";
            case 0x02:
                return "ZDO_NODE_DESC_REQ";
            case 0x06:
                return "ZDO_MATCH_DESC_REQ";
            case 0x21:
                return "ZDO_BIND_REQ";
            case 0x22:
                return "ZDO_UNBIND_REQ";
            case 0x36:
                return "ZDO_MGMT_PERMIT_JOIN_REQ"; // "Permit Join"
            case 0x40:
                return "ZDO_STARTUP_FROM_APP"; // "Start Network"
            case 0xC1:
                return "ZDO_END_DEVICE_ANNCE_IND"; // "Device Joined"
            case 0xCA:
                return "ZDO_TC_DEV_IND"; // "Trust Center: New Device"
            case 0x80:
                return "ZDO_STATE_CHANGE_IND"; // "Network State Changed"
            }
        }

        // 4. UTIL COMMANDS
        if ((cmd0 & 0x1F) == 0x07)
        {
            if (cmd1 == 0x00)
                return "UTIL_GET_DEVICE_INFO";
        }

        // Default: Return Raw Hex if unknown
        return "UNKNOWN (" + toHex(cmd0, 2) + ", " + toHex(cmd1, 2) + ")";
    }

    inline std::string getZCLCommandName(uint8_t cmdId)
    {
        auto it = zclCommandNameMap.find(cmdId);
        if (it != zclCommandNameMap.end())
        {
            return it->second;
        }

        std::stringstream ss;
        ss << "Unknown ZCL Command: " << toHex(cmdId, 2);
        return ss.str();
    }

    inline std::string getClusterName(uint16_t clusterId)
    {
        auto it = clusterNameMap.find(clusterId);
        if (it != clusterNameMap.end())
        {
            return it->second;
        }
        std::stringstream ss;
        ss << "Unknown Cluster: " << toHex(clusterId, 4);
        return ss.str();
    }
};

#endif // ZSTACK_PROTOCOL_H