#include <stdio.h>
#include <pico/stdlib.h>
#include "lib/uart_pico.h"
#include "lib/uart_rx.h"
#include "lib/uart_tx.h"

#define rxGPS 7
#define txGPS 8

UartPico uart_pico = UART_PICO();
UartRx *uart_rx;
UartTx *uart_tx;

void setup()
{
    stdio_init_all();
    while (!stdio_usb_connected())
    {
        tight_loop_contents(); // Wait for USB serial connection to be ready
    }
    sleep_ms(100);

    uart_tx = UartTx_init(&uart_pico, txGPS);
    UartTx_activate(uart_tx); // connect gps sensor

    uart_rx = UartRx_init(&uart_pico, rxGPS);
    UartRx_activate(uart_rx); // connect gps sensor
    printf("GPS Module Initialized.\n");
}

// Variable to track time
unsigned long lastTime = 0;
unsigned long pauseInterval = 10000; // 20 seconds (in milliseconds)
bool isPaused = false;               // Flag to track whether it's in paused state

void loop()
{
    // Get current time
    unsigned long currentTime = time_us_32() / 1000;

    // If 20 seconds have passed
    if (currentTime - lastTime >= pauseInterval)
    {
        if (isPaused)
        {
            // If currently paused, resume
            UartTx_println(uart_tx, "$PMTK161,0*29");
        }
        else
        {
            // If currently running, pause
            UartTx_println(uart_tx, "$PMTK161,0*28");
        }

        // Toggle paused state
        isPaused = !isPaused;

        // Reset lastTime to current time
        lastTime = currentTime;
    }

    while (UartRx_available(uart_rx))
    {
        char *gpsData = UartRx_readStringUntil(uart_rx, '\n');
        if (gpsData[0] == '$')
        {
            printf("\n");
        }
        printf(gpsData);
    }
}

int main()
{
    setup();
    while (true)
    {
        loop();
    }
    return 0;
}
