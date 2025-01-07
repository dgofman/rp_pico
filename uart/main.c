#include <stdio.h>
#include <pico/stdlib.h>
#include "uart_pico.h"
#include "uart_rx.h"
#include "uart_tx.h"

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

    uart_tx = UartTx_init(&uart_pico, RXD2TX);
    UartTx_activate(uart_tx); // connect gps sensor

    uart_rx = UartRx_init(&uart_pico, TXD2RX);
    UartRx_activate(uart_rx); // connect gps sensor
    printf("GPS Module Initialized.\n");
}

// Variable to track time
unsigned long lastTime = 0;
unsigned long pauseInterval = 10000; // Interval for pausing the GPS module (10 seconds)
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
            printf("Resume\n");
            UartTx_println(uart_tx, "$PMTK161,0*29");
        }
        else
        {
            // If currently running, pause
            printf("Pause\n");
            UartTx_println(uart_tx, "$PMTK161,0*28");
        }

        // Toggle paused state
        isPaused = !isPaused;

        // Reset lastTime to current time
        lastTime = currentTime;
    }

    while (UartRx_available(uart_rx))
    {
        char *gpsData = UartRx_readLine(uart_rx);
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
