#ifndef SERIAL_PORT_H
#define SERIAL_PORT_H

#include <string>
#include <vector>

class SerialPort {
public:
    // Constructor: Takes the path (e.g., "/dev/ttyUSB0")
    SerialPort(const std::string& portName);
    
    // Destructor: Automatically closes the port when the object is destroyed
    ~SerialPort();

    // Core functions
    bool openPort();
    void closePort();
    
    // Send raw bytes (for the Z-Stack protocol)
    int writeBytes(const std::vector<unsigned char>& data);
    
    // Read raw bytes
    int readBytes(std::vector<unsigned char>& buffer);

private:
    std::string portName;
    int fileDescriptor; // The ID Linux gives the open file
    bool isConnected;
    
    // Helper to configure termios (Baud rate, Parity, etc.)
    bool configureTermios(); 
};

#endif // SERIAL_PORT_H