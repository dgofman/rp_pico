#include "GPS.h"
#include "../nmea/nmea_parser.h"

#include <stdio.h>
#include <pico/stdlib.h>

// https://www.dragino.com/downloads/downloads/datasheet/other_vendors/L80-R/Quectel_L80-R_Hardware_Design_V1.2.pdf
/*
The module provides one universal asynchronous receiver & transmitter serial port. The module is
designed as DCE (Data Communication Equipment), following the traditional DCE-DTE (Data Terminal
Equipment) connection. The module and the client (DTE) are connected through the signals shown in
following figure. It supports data baud-rate from 4800bps to 115200bps.
UART port:
* TXD1: Send data to the RXD signal line of DTE.
* RXD1: Receive data from the TXD signal line of DTE
*/

// Pin assignments for GPS
#define TXD2RX 3 // GPS TXD (module transmit) -> SPI0 RX (microcontroller receive)
#define RXD2TX 4 // GPS RXD (module receive)  -> SPI0 TX (microcontroller transmit)

// Declare a global pointer to the GPS object
GPS *gps;

/**
 * @brief Setup function for initializing the GPS module.
 *
 * This function sets up the serial interface, waits for the USB connection, initializes
 * the GPS module, and configures the GPS to work with specific intervals and settings.
 */
void setup()
{
    stdio_init_all();              // Initialize the standard input/output
    while (!stdio_usb_connected()) // Wait for the USB serial connection to be established
    {
        tight_loop_contents(); // Perform an empty loop to wait for USB serial connection
    }
    sleep_ms(100); // Short delay to ensure proper USB connection

    // Initialize the GPS object with default intervals for sentence output
    gps = new GPS(DEFAULT_INTERVALS()); // By default: .GSA = false and .GSV = false
    gps->init(TXD2RX, RXD2TX);          // Initialize GPS with RX and TX pin configurations
    gps->setDelay(5);                   // Set the GPS update delay to 5 seconds (200 millihertz).
                                        // For optimal performance, it's recommended to use a delay of 1 second (1 Hz) or higher.
    printf("GPS Module Initialized.\n");
}

unsigned long lastTime = 0;          // Variable to store the last time the GPS state was toggled
unsigned long pauseInterval = 10000; // Interval for pausing the GPS module (10 seconds)
bool isPaused = false;               // Flag to track whether the GPS module is in paused state

/**
 * @brief Main loop function to read and display GPS data.
 *
 * The loop checks if it's time to pause or wake up the GPS module based on the interval,
 * reads the GPS data, and prints the results (latitude, longitude, speed, etc.) to the console.
 */
void loop()
{
    // Get current time in milliseconds
    unsigned long currentTime = time_us_32() / 1000;

    // Check if the time to toggle the GPS state has elapsed
    if (currentTime - lastTime >= pauseInterval)
    {
        // Toggle the GPS state between standby and wakeup
        if (isPaused)
        {
            gps->wakeup(); // Wake up the GPS module
        }
        else
        {
            gps->standby(); // Put the GPS module into standby mode
        }

        // Toggle the paused state
        isPaused = !isPaused;

        // Update the last time with the current time
        lastTime = currentTime;
    }

    // Read and process GPS data if available
    while (gps->isAvailable())
    {
        // Read a new GPS sentence
        char *result = gps->read();
        if (result == NULL) // Skip if no data is available
        {
            continue;
        }

        // Print the raw NMEA sentence
        printf("\n%s", result);

        // Display parsed GPS data (date, latitude, longitude, speed)
        printf("Date: %d-%d-%d\n", gps->year(), gps->month(), gps->day());
        printf("Latitude %f\n", gps->latitude());
        printf("Longitude %f\n", gps->longitude());
        printf("Speed %f\n", gps->speed());
    }
}

/**
 * @brief The main entry point of the program.
 *
 * This function sets up the GPS module and enters an infinite loop to continuously
 * read and display GPS data. It calls the setup function to initialize everything
 * and then continuously calls the loop function.
 *
 * @return 0 when the program finishes execution (although this never happens in this case).
 */
int main()
{
    setup(); // Call the setup function to initialize the GPS and other settings
    while (true)
    {
        loop(); // Continuously call the loop function to process GPS data
    }
    return 0; // Return 0 (although this line is never reached)
}
