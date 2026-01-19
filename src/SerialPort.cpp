#include "SerialPort.h"
#include <iostream>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include "Logger.h"

SerialPort::SerialPort(const std::string& portName) 
    : portName(portName), fileDescriptor(-1), isConnected(false) {}

SerialPort::~SerialPort() {
    closePort();
}

bool SerialPort::openPort() {
    // Open in Read/Write, No controlling TTY, Non-blocking initially
    fileDescriptor = open(portName.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    
    if (fileDescriptor < 0) {
        std::cerr << "Error opening " << portName << ": " << strerror(errno) << std::endl;
        return false;
    }

    if (!configureTermios()) {
        return false;
    }

    isConnected = true;
    std::cout << "Successfully connected to " << portName << std::endl;
    return true;
}

void SerialPort::closePort() {
    if (isConnected && fileDescriptor >= 0) {
        close(fileDescriptor);
        isConnected = false;
        fileDescriptor = -1;
        std::cout << "Port closed." << std::endl;
    }
}

bool SerialPort::configureTermios() {
    struct termios tty;
    if (tcgetattr(fileDescriptor, &tty) != 0) {
        std::cerr << "Error from tcgetattr: " << strerror(errno) << std::endl;
        return false;
    }

    // --- CONFIGURATION FOR SONOFF DONGLE P (TI CC2652P) ---
    
    // 1. Control Modes
    tty.c_cflag &= ~PARENB;        // No Parity
    tty.c_cflag &= ~CSTOPB;        // One stop bit
    tty.c_cflag |= CS8;            // 8 bits per byte
    tty.c_cflag &= ~CRTSCTS;       // DISABLE Flow Control (Crucial for Sonoff P)
    tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines

    // 2. Local Modes (Raw mode)
    tty.c_lflag &= ~ICANON; // Disable canonical mode
    tty.c_lflag &= ~ECHO;   // Disable echo
    tty.c_lflag &= ~ECHOE;  // Disable erasure
    tty.c_lflag &= ~ISIG;   // Disable signals

    // 3. Input Modes
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);

    // 4. Output Modes
    tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes
    tty.c_oflag &= ~ONLCR; 

    // 5. Timeouts
    tty.c_cc[VTIME] = 1; // Wait up to 100ms
    tty.c_cc[VMIN] = 0;

    // 6. Baud Rate (115200 is standard for Z-Stack 3.x)
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    if (tcsetattr(fileDescriptor, TCSANOW, &tty) != 0) {
        std::cerr << "Error from tcsetattr: " << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

int SerialPort::writeBytes(const std::vector<unsigned char>& data) {
    if (!isConnected) return -1;
    return write(fileDescriptor, data.data(), data.size());
}

int SerialPort::readBytes(std::vector<unsigned char>& buffer) {
    if (!isConnected) return -1;
    
    unsigned char temp_buf[256];
    memset(&temp_buf, '\0', sizeof(temp_buf));
    
    int num_bytes = read(fileDescriptor, &temp_buf, sizeof(temp_buf));
    
    if (num_bytes > 0) {
        buffer.assign(temp_buf, temp_buf + num_bytes);
    }
    return num_bytes;
}