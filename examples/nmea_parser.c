#include <stdio.h>
#include <pico/stdlib.h>
#include "gps/nmea_parser.h"

#define rxGPS 7
#define txGPS 8

NMEAParser nmeaParser;

void setup()
{
    stdio_init_all();
    while (!stdio_usb_connected())
    {
        tight_loop_contents(); // Wait for USB serial connection to be ready
    }
    sleep_ms(100);

    NMEAParser_init(&nmeaParser, rxGPS, txGPS);
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