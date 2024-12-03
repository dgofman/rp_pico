cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

# Define project name as a variable
set(PROJECT_NAME gps_reader)

# Set the project name
project(${PROJECT_NAME} C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

# Specify the source files
add_executable(${PROJECT_NAME}
    main.c
)

# Include the uart directory for header files
file(GLOB UART_SOURCES "lib/*.c")
target_sources(${PROJECT_NAME} PRIVATE ${UART_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE lib)

# Link the necessary libraries
target_link_libraries(${PROJECT_NAME} pico_stdlib hardware_pio)

# Enable USB output, disable UART output
pico_enable_stdio_usb(${PROJECT_NAME} 1)

# Create a UF2 file for uploading to the RP2040
pico_add_extra_outputs(${PROJECT_NAME})

# Optional: Set verbosity for debugging the build process
# set(CMAKE_VERBOSE_MAKEFILE ON)