# Define the project name as uart
set(PROJECT_NAME uart)

# Create an executable target from the main source file
add_executable(${PROJECT_NAME} main.c)

# Link the executable to the top-level library and other required libraries
target_link_libraries(${PROJECT_NAME} rp_pico)

# Enable USB output (STDIO will be over USB rather than UART)
pico_enable_stdio_usb(${PROJECT_NAME} 1)

# Add additional outputs (e.g., UF2 bootloader, etc.)
pico_add_extra_outputs(${PROJECT_NAME})
