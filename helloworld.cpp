#include <iostream>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>

int main() {
    // 1. Open the serial port
    int serialPort = open("/dev/ttyUSB0", O_RDWR);

    if (serialPort < 0) {
        std::cerr << "Error " << errno << " opening /dev/ttyUSB0: " << strerror(errno) << std::endl;
        return 1;
    }

    std::cout << "Serial port opened successfully." << std::endl;

    // 2. Configure the serial port
    struct termios tty;
    
    if (tcgetattr(serialPort, &tty) != 0) {
        std::cerr << "Error " << errno << " from tcgetattr: " << strerror(errno) << std::endl;
        return 1;
    }

    // ---- Control Modes ---
    tty.c_cflag &= ~PARENB; // Clear parity bit , disabling parity (most common)
    tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
    tty.c_cflag &= ~CSIZE;  // Clear all bits that set the data size 
    tty.c_cflag |= CS8;     // 8 bits per byte (most common)
    tty.c_cflag &= ~CRTSCTS; // Enable RTS/CTS hardware flow control (most common)
    tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)
    
    // ---- Local Modes ----
    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO; // Disable echo
    tty.c_lflag &= ~ECHOE; // Disable erasure
    tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP

    // ---- Input Modes ----
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes    

    // ---- Output Modes ----
    tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

    // --- Blocking / Timeouts ---
    tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
    tty.c_cc[VMIN] = 0;             // No minimum number of characters for non-canonical read.

    // ---- Set Baud Rate ----
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    // Save tty settings, also checking for error
    if (tcsetattr(serialPort, TCSANOW, &tty) != 0) {
        std::cerr << "Error " << errno << " from tcsetattr: " << strerror(errno) << std::endl;
        return 1;
    }

    // 3. Write to serial port
    unsigned char command[] = { 0xFE, 0x00, 0x21, 0x02, 0x23 }; 
    
    int bytes_written = write(serialPort, command, sizeof(command));
    
    std::cout << "Sent " << bytes_written << " bytes: FE 00 21 02 23" << std::endl;

    // 4. Close the serial port
// 4. READ FROM SERIAL PORT
    unsigned char read_buf [256];
    memset(&read_buf, '\0', sizeof(read_buf));

    std::cout << "Waiting for response (timeout 1s)..." << std::endl;
    int num_bytes = read(serialPort, &read_buf, sizeof(read_buf));

    if (num_bytes < 0) {
        std::cerr << "Error reading: " << strerror(errno) << std::endl;
        return 1;
    }

    // --- NEW: Print as HEX instead of String ---
    std::cout << "Read " << num_bytes << " bytes: ";
    for (int i = 0; i < num_bytes; i++) {
        // Cast to unsigned char to avoid negative printing for bytes > 127
        printf("%02X ", (unsigned char)read_buf[i]);
    }
    std::cout << std::endl;
    // -------------------------------------------

    close(serialPort);
    return 0;
}