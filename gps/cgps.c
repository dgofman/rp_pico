#include "cgps.h"
#include "../uart/uart_rx.h"
#include "../uart/uart_tx.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

/**
 * @brief Updates the NMEA sentence output intervals for the GPS module.
 *
 * @param gps - The GPS object.
 * @param enable - A boolean value indicating whether to enable or disable the intervals.
 */
static void _doUpdateIntervals(CGPS *gps, int enable)
{
    char command[100];
    snprintf(command, sizeof(command),
             "$PMTK314,%d,%d,%d,%d,%d,%d,0,0,0,0,0,0,0,0,0,0,0,0,0*%s",
             gps->intervals.GLL ? 1 : 0,
             gps->intervals.RMC ? 1 : 0,
             gps->intervals.VTG ? 1 : 0,
             gps->intervals.GGA ? 1 : 0,
             gps->intervals.GSA ? 1 : 0,
             gps->intervals.GSV ? 1 : 0,
             enable ? "28" : "29"); // Command format with checksum (enable/disable)
    GPS_write(gps, command);
}

/**
 * @brief Converts a latitude or longitude in DDMM format to decimal degrees.
 * @param ddmm - The coordinate in DDMM format.
 * @return A double representing the coordinate in decimal degrees.
 */
static double _convertToDecimalDegrees(double ddmm)
{
    int degrees = (int)(ddmm / 100);             // Extract degrees
    double lat_mm_mmmm = ddmm - (degrees * 100); // Extract minutes
    int minutes = (int)lat_mm_mmmm;
    double fractionalMinutes = lat_mm_mmmm - minutes;

    double minutesDecimal = (minutes + fractionalMinutes) / 60.0; // Convert minutes to decimal
    return degrees + minutesDecimal;                              // Return decimal degrees
}

/**
 * @brief Initializes the GPS module with specified RX and TX pins.
 * @param gps - The GPS object.
 * @param intervals - The configuration for sentence intervals.
 * @param rx - The RX pin number.
 * @param tx - The TX pin number.
 */
void GPS_init(CGPS *gps, IntervalType intervals, int rx, int tx)
{
    gps->intervals = intervals;
    gps->start_year = 2000;
    NMEAParser_init(&gps->nmeaParser, rx, tx);
    GPS_updateIntervals(gps);
}

/**
 * @brief Destructor that frees allocated resources.
 * @param gps - The GPS object.
 */
void GPS_free(CGPS *gps)
{
    NMEAParser_free(&gps->nmeaParser);
}

/**
 * @brief Checks if the GPS data is available.
 * @param gps - The GPS object.
 * @return 1 if data is available, 0 otherwise.
 */
int GPS_isAvailable(CGPS *gps)
{
    return NMEAParser_available(&gps->nmeaParser) > 0;
}

/**
 * @brief Reads GPS data from the NMEA parser.
 * @param gps - The GPS object.
 * @return A pointer to the NMEA sentence data.
 */
char *GPS_read(CGPS *gps)
{
    return NMEAParser_read(&gps->nmeaParser);
}

/**
 * @brief Sends a command string to the GPS module.
 * @param gps - The GPS object.
 * @param str - The NMEA command string to send.
 */
void GPS_write(CGPS *gps, const char *str)
{
    UartTx_println(gps->nmeaParser.uart_tx, str);
}

/**
 * @brief Retrieves the current GPS data.
 * @param gps - The GPS object.
 * @return A constant pointer to the GPSData object.
 */
const GPSData *GPS_getGPSData(CGPS *gps)
{
    return &gps->nmeaParser.data;
}

/**
 * @brief Gets the latitude in decimal degrees.
 * @param gps - The GPS object.
 * @return The latitude value in decimal degrees.
 */
double GPS_latitude(CGPS *gps)
{
    GPSData data = *GPS_getGPSData(gps);
    if (data.latitude > 0 && data.latitude_dir != NULL)
    {
        double decimalDegrees = _convertToDecimalDegrees(data.latitude);
        if (strcmp(data.latitude_dir, "S") == 0)
        {
            decimalDegrees = -decimalDegrees;
        }
        return decimalDegrees;
    }
    return 0;
}

/**
 * @brief Gets the longitude in decimal degrees.
 * @param gps - The GPS object.
 * @return The longitude value in decimal degrees.
 */
double GPS_longitude(CGPS *gps)
{
    GPSData data = *GPS_getGPSData(gps);
    if (data.longitude > 0 && data.longitude_dir != NULL)
    {
        double decimalDegrees = _convertToDecimalDegrees(data.longitude);
        if (strcmp(data.longitude_dir, "W") == 0)
        {
            decimalDegrees = -decimalDegrees;
        }
        return decimalDegrees;
    }
    return 0;
}

/**
 * @brief Retrieves the current date in YYYYMMDD format.
 * @param gps - The GPS object.
 * @return The current date as a long integer.
 */
long GPS_getDate(CGPS *gps)
{
    return atol(GPS_getGPSData(gps)->gprmc.date);
}

/**
 * @brief Gets the year from the GPS date.
 * @param gps - The GPS object.
 * @return The year part of the current date.
 */
unsigned int GPS_year(CGPS *gps)
{
    return (GPS_getDate(gps) % 100) + gps->start_year;
}

/**
 * @brief Gets the month from the GPS date.
 * @param gps - The GPS object.
 * @return The month part of the current date.
 */
unsigned char GPS_month(CGPS *gps)
{
    return (GPS_getDate(gps) / 100) % 100;
}

/**
 * @brief Gets the day from the GPS date.
 * @param gps - The GPS object.
 * @return The day part of the current date.
 */
unsigned char GPS_day(CGPS *gps)
{
    return GPS_getDate(gps) / 10000;
}

/**
 * @brief Retrieves the ground speed in knots.
 * @param gps - The GPS object.
 * @return The ground speed in knots.
 */
float GPS_speed(CGPS *gps)
{
    return GPS_getGPSData(gps)->speed;
}

/**
 * @brief Updates the NMEA sentence output intervals.
 * @param gps - The GPS object.
 */
void GPS_updateIntervals(CGPS *gps)
{
    _doUpdateIntervals(gps, 0); // Disable intervals first
    _doUpdateIntervals(gps, 1); // Then enable them
}

/**
 * @brief Sets the GPS position update frequency.
 * @param gps - The GPS object.
 * @param hz - The desired frequency in Hertz (Hz).
 */
void GPS_setFrequency(CGPS *gps, double hz)
{
    char nmea[50], checksum_str[3], command[60];
    int interval = (int)(1000 / hz); // Convert Hz to interval in milliseconds
    snprintf(nmea, sizeof(nmea), "PMTK220,%d", interval);

    unsigned char checksum = 0;
    for (int i = 0; nmea[i] != '\0'; i++)
    {
        checksum ^= nmea[i];
    }
    snprintf(checksum_str, sizeof(checksum_str), "%02X", checksum);
    snprintf(command, sizeof(command), "$%s*%s", nmea, checksum_str);
    GPS_write(gps, command);
}

/**
 * @brief Sets the GPS position fix delay.
 * @param gps - The GPS object.
 * @param seconds - The delay in seconds (maximum is 10 seconds).
 */
void GPS_setDelay(CGPS *gps, unsigned short seconds)
{
    GPS_setFrequency(gps, 1.0 / seconds);
}

/**
 * @brief Puts the GPS module in standby mode.
 * @param gps - The GPS object.
 */
void GPS_standby(CGPS *gps)
{
    GPS_write(gps, "$PMTK161,0*28");
}

/**
 * @brief Wakes up the GPS module from standby mode.
 * @param gps - The GPS object.
 */
void GPS_wakeup(CGPS *gps)
{
    GPS_write(gps, "$PMTK161,0*29");
}
