#ifndef CGPS_H
#define CGPS_H

#include "nmea_parser.h"

#ifdef __cplusplus
extern "C"
{
#endif

// Default response for setting the NMEA output.
#define PMTK_RESPONSE "$PMTK001,314,3*36\r\n" // 3.23. Packet Type: 314 PMTK_API_SET_NMEA_OUTPUT

// Default intervals for various NMEA sentence types.
#define DEFAULT_INTERVALS() \
    {                       \
        .GLL = 1,           \
        .RMC = 1,           \
        .VTG = 1,           \
        .GGA = 1,           \
        .GSA = 0,           \
        .GSV = 0,           \
    }

    /**
     * @brief Represents the configuration for NMEA sentence intervals.
     *        Controls which NMEA sentences are enabled and their frequency.
     */
    typedef struct
    {
        int GLL; // Geographic Position - Latitude longitude
        int RMC; // Recommended Minimum Specific GPS Sentence
        int VTG; // Course Over Ground and Ground Speed
        int GGA; // GPS Fix Data
        int GSA; // GPS DOPS and Active Satellites
        int GSV; // GPS Satellites in View
    } IntervalType;

    /**
     * @brief GPS object structure that encapsulates the interaction with a GPS module.
     *        Handles communication, parsing, and NMEA sentence interval configuration.
     */
    typedef struct
    {
        NMEAParser nmeaParser;   /**< Pointer to the NMEA parser object. */
        IntervalType intervals;  /**< Stores the sentence intervals configuration. */
        unsigned int start_year; /**< The base year (2000) for date calculation. */
    } CGPS;

    /**
     * Function declarations
     */
    void GPS_init(CGPS *gps, IntervalType intervals, int rx, int tx);
    void GPS_free(CGPS *gps);
    int GPS_isAvailable(CGPS *gps);
    char *GPS_read(CGPS *gps);
    void GPS_write(CGPS *gps, const char *str);
    const GPSData *GPS_getGPSData(CGPS *gps);
    double GPS_latitude(CGPS *gps);
    double GPS_longitude(CGPS *gps);
    long GPS_getDate(CGPS *gps);
    unsigned int GPS_year(CGPS *gps);
    unsigned char GPS_month(CGPS *gps);
    unsigned char GPS_day(CGPS *gps);
    float GPS_speed(CGPS *gps);

    void GPS_updateIntervals(CGPS *gps);
    void GPS_setFrequency(CGPS *gps, double hz);
    void GPS_setDelay(CGPS *gps, unsigned short seconds);
    void GPS_standby(CGPS *gps);
    void GPS_wakeup(CGPS *gps);

#ifdef __cplusplus
}
#endif

#endif // CGPS_H
