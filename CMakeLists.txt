cmake_minimum_required(VERSION 3.10)
project(ZigbeeDriver)

set(CMAKE_CXX_STANDARD 17)

# 1. Include directories (Header files)
include_directories(include)

# 2. Create the Library (The "Engine")
# We compile SerialPort.cpp into a static library named 'zigbee_core'
add_library(zigbee_core src/SerialPort.cpp src/ZStackFrame.cpp src/ZStackParser.cpp src/ZStackClient.cpp src/af/AFPacketParser.cpp src/zdo/ZDOPacketParser.cpp)


# 3. Create the Executable (The "Car")
# We compile main.cpp into an executable named 'zigbee_test'
add_executable(zigbee_test src/main.cpp)

# 4. Link them together
# Connect the engine to the car
target_link_libraries(zigbee_test zigbee_core)