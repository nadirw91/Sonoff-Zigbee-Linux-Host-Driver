cmake_minimum_required(VERSION 3.10)
project(ZigbeeDriver)

set(CMAKE_CXX_STANDARD 17)

# 1. Include directories (Header files)
include_directories(include)

# 2. Create the Library (The "Engine")
# We compile SerialPort.cpp into a static library named 'zigbee_sdk'
add_library(zigbee_sdk src/SerialPort.cpp src/ZStackFrame.cpp src/ZStackParser.cpp src/ZStackClient.cpp src/af/AFPacketParser.cpp src/zdo/ZDOPacketParser.cpp)

# 2. Define Include Directories
# "PUBLIC" means: "I need this folder to build, AND anyone using me needs it too"
target_include_directories(zigbee_sdk PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# "PRIVATE" means: "Only I need this folder (internal parsers)"
target_include_directories(zigbee_sdk PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 3. Installation Rules (This creates the "package")
# This tells CMake where to put the files when you run 'make install'

# Install the binary (.a file)
install(TARGETS zigbee_sdk DESTINATION lib)

# Install the public headers
install(DIRECTORY include/ DESTINATION .)

# 3. Create the Executable (The "Car")
# We compile main.cpp into an executable named 'zigbee_test'
add_executable(zigbee_test src/main.cpp)

# 4. Link them together
# Connect the engine to the car
target_link_libraries(zigbee_test zigbee_sdk)