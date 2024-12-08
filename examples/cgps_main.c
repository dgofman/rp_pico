#include "gps/cgps.h"
#include "gps/nmea_parser.h"

#include <stdio.h>
#include <pico/stdlib.h>

// Define RX and TX pins for GPS module communication
#define rxGPS 7
#define txGPS 8

// Declare a global pointer to the GPS object
CGPS gps;

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
    IntervalType intervals = DEFAULT_INTERVALS();
    GPS_init(&gps, intervals, rxGPS, txGPS);
    GPS_setDelay(&gps, 5); // Set the GPS update delay to 5 seconds (200 millihertz).
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
            GPS_wakeup(&gps); // Wake up the GPS module
        }
        else
        {
            GPS_standby(&gps); // Put the GPS module into standby mode
        }

        // Toggle the paused state
        isPaused = !isPaused;

        // Update the last time with the current time
        lastTime = currentTime;
    }

    // Read and process GPS data if available
    while (GPS_isAvailable(&gps))
    {
        // Read a new GPS sentence
        char *result = GPS_read(&gps);
        if (result == NULL) // Skip if no data is available
        {
            continue;
        }

        // Print the raw NMEA sentence
        printf("\n%s", result);

        // Display parsed GPS data (date, latitude, longitude, speed)
        printf("Date: %d-%d-%d\n", GPS_year(&gps), GPS_month(&gps), GPS_day(&gps));
        printf("Latitude %f\n", GPS_latitude(&gps));
        printf("Longitude %f\n", GPS_longitude(&gps));
        printf("Speed %f\n", GPS_speed(&gps));
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
