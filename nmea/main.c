#include <stdio.h>
#include <pico/stdlib.h>
#include "nmea_parser.h"

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

NMEAParser nmeaParser;

void setup()
{
    stdio_init_all();
    while (!stdio_usb_connected())
    {
        tight_loop_contents(); // Wait for USB serial connection to be ready
    }
    sleep_ms(100);

    NMEAParser_init(&nmeaParser, TXD2RX, RXD2TX);
    // nmeaParser.GPGGA_ENABLED = false;
    // nmeaParser.GPGLL_ENABLED = false;
    // nmeaParser.GPRMC_ENABLED = false;
    // nmeaParser.GPGSA_ENABLED = false;
    // nmeaParser.GPVTG_ENABLED = false;
    nmeaParser.GPGSV_ENABLED = false;

    printf("GPS Module Initialized.\n");
}

void loop()
{
    while (NMEAParser_available(&nmeaParser))
    {
        char *result = NMEAParser_read(&nmeaParser);
        if (result != NULL)
        {
            printf("\n%s\n", result);
        }

        // Print the parsed data
        GPGGA_Data *gpgga = &nmeaParser.data.gpgga;
        if (gpgga->last_time)
        {
            printf("Time: %s\n", gpgga->utc_time);
            printf("Latitude: %f\n", gpgga->latitude);
            printf("Latitude Dir: %s\n", gpgga->latitude_dir);
            printf("Longitude: %f\n", gpgga->longitude);
            printf("Longitude Dir: %s\n", gpgga->longitude_dir);
            printf("Fix status: %d\n", gpgga->fix_status);
            printf("Number of satellites: %d\n", gpgga->num_satellites);
            printf("HDOP: %f\n", gpgga->hdop);
            printf("Altitude: %f\n", gpgga->altitude);
            printf("Altitude Unit: %s\n", gpgga->altitude_unit);
            printf("Geoid: %f\n", gpgga->geoid_separation);
            printf("Geoid Unit: %s\n", gpgga->geoid_unit);
            printf("Last Time: %d\n", gpgga->last_time);
            gpgga->last_time = 0; // Reset last_time to 0 to indicate that the GPGGA data has been processed
        }

        GPGLL_Data *gpgll = &nmeaParser.data.gpgll;
        if (gpgll->last_time)
        {
            printf("Time: %s\n", gpgll->utc_time);
            printf("Latitude: %f\n", gpgll->latitude);
            printf("Latitude Dir: %s\n", gpgll->latitude_dir);
            printf("Longitude: %f\n", gpgll->longitude);
            printf("Longitude Dir: %s\n", gpgll->longitude_dir);
            printf("Status indicator: %s\n", gpgll->status);
            gpgll->last_time = 0; // Reset last_time to 0 to indicate that the GPGLL data has been processed
        }

        GPRMC_Data *gprmc = &nmeaParser.data.gprmc;
        if (gprmc->last_time && strcmp(gprmc->status, "A") == 0)
        {
            printf("Date: %s\n", gprmc->date);
            printf("Time: %s\n", gprmc->utc_time);
            printf("Latitude: %f\n", gprmc->latitude);
            printf("Latitude Dir: %s\n", gprmc->latitude_dir);
            printf("Longitude: %f\n", gprmc->longitude);
            printf("Longitude Dir: %s\n", gprmc->longitude_dir);
            printf("Status: %s\n", gprmc->status);
            printf("Speed (Knots): %f\n", gprmc->speed);
            printf("Track (True): %f\n", gprmc->track);
            printf("Magnetic variation: %f\n", gprmc->variation);
            gprmc->last_time = 0; // Reset last_time to 0 to indicate that the GPRMC data has been processed
        }

        GPGSA_Data *gpgsa = &nmeaParser.data.gpgsa;
        if (gpgsa->last_time)
        {
            printf("Mode 1: %s\n", gpgsa->mode1);
            printf("Mode 2: %d\n", gpgsa->mode2);
            printf("PRN number: %d\n", gpgsa->prn);
            printf("PDOP: %d\n", gpgsa->pdop);
            printf("HDOP: %d\n", gpgsa->hdop);
            printf("VDOP: %d\n", gpgsa->vdop);
            gpgsa->last_time = 0; // Reset last_time to 0 to indicate that the GPGSA data has been processed
        }

        GPVTG_Data *gpvtg = &nmeaParser.data.gpvtg;
        if (gpvtg->last_time)
        {
            printf("Track (degrees true): %f\n", gpvtg->track1);
            printf("T - (true north): %s\n", gpvtg->track1_id);
            printf("Track (degrees magnetic): %f\n", gpvtg->track2);
            printf("M - (magnetic north): %s\n", gpvtg->track2_id);
            printf("Speed (in knots): %f\n", gpvtg->speed1);
            printf("N - (in knots): %s\n", gpvtg->speed1_id);
            printf("Speed (in kph): %f\n", gpvtg->speed2);
            printf("K - (in kph): %s\n", gpvtg->speed2_id);
            gpvtg->last_time = 0; // Reset last_time to 0 to indicate that the GPVTG data has been processed
        }

        GPGSV_Data *gpgsv = &nmeaParser.data.gpgsv;
        if (gpgsv->last_time)
        {
            printf("Total number of messages: %d\n", gpgsv->total);
            printf("Message number: %d\n", gpgsv->count);
            printf("Total number of SVs: %d\n", gpgsv->total_sv);
            printf("SV PRN number: %d\n", gpgsv->prn_sv);
            printf("Elevation, in degrees: %d\n", gpgsv->elevation);
            printf("Azimuth, degrees: %d\n", gpgsv->azimuth);
            printf("SNR: %d\n", gpgsv->snr);
            gpgsv->last_time = 0; // Reset last_time to 0 to indicate that the GPGSV data has been processed
        }
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