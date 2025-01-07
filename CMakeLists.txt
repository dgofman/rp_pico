cmake_minimum_required(VERSION 3.12)

# Include the Pico SDK
include(pico_sdk_import.cmake)

# Define the project name
set(PROJECT_NAME rp_pico)

# Set the project name
project(${PROJECT_NAME} C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

# Define the top-level library (static library)
add_library(${PROJECT_NAME} STATIC 
    uart/uart_rx.c
    uart/uart_tx.c
    nmea/nmea_parser.c
    gps/cgps.c
    # gps/GPS.cpp # gps/CMakeLists.txt -> add_executable(${PROJECT_NAME} main.cpp)
)

# Specify the include directories for the top-level library
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} uart nmea gps)

# Link libraries that the top-level project might need
target_link_libraries(${PROJECT_NAME} pico_stdlib hardware_pio)

# Add subdirectories for other components
add_subdirectory(uart)
add_subdirectory(nmea)
add_subdirectory(gps)