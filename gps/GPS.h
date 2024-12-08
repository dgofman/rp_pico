#ifndef GPS_H
#define GPS_H

#include "nmea_parser.h"

// Default response for setting the NMEA output.
#define PMTK_RESPONSE "$PMTK001,314,3*36\r\n" // 3.23. Packet Type: 314 PMTK_API_SET_NMEA_OUTPUT

// Default intervals for various NMEA sentence types.
#define DEFAULT_INTERVALS() \
    {                       \
        .GLL = true,        \
        .RMC = true,        \
        .VTG = true,        \
        .GGA = true,        \
        .GSA = false,       \
        .GSV = false,       \
    }

/**
 * @brief Represents the configuration for NMEA sentence intervals.
 *        Controls which NMEA sentences are enabled and their frequency.
 */
typedef struct
{
    bool GLL; // Geographic Position - Latitude longitude
    bool RMC; // Recommended Minimum Specific GPS Sentence
    bool VTG; // Course Over Ground and Ground Speed
    bool GGA; // GPS Fix Data
    bool GSA; // GPS DOPS and Active Satellites
    bool GSV; // GPS Satellites in View
} IntervalType;

/**
 * @brief GPS class that encapsulates the interaction with a GPS module.
 *        Handles communication, parsing, and NMEA sentence interval configuration.
 */
class GPS
{
private:
    NMEAParser *_nmeaParser; /**< Pointer to the NMEA parser object. */
    // Helper functions
    void _doUpdateIntervals(bool enable);
    double _convertToDecimalDegrees(double ddmm);

public:
    IntervalType intervals; /**< Stores the sentence intervals configuration. */
    uint start_year;        /**< The base year (2000) for date calculation. */
    GPS(IntervalType intervals);
    ~GPS();
    void init(int rx, int tx);
    bool isAvailable();
    char *read();
    void write(const char *str);
    const GPSData &getGPSData() const;
    double latitude();
    double longitude();
    const long getDate();
    uint16_t year();
    uint8_t month();
    uint8_t day();
    float speed();

    // https://www.dragino.com/downloads/downloads/datasheet/other_vendors/L80/Quectel_L80_GPS_Protocol_Specification_V1.3.pdf
    void updateIntervals();
    void setFrequency(double hz);
    void setDelay(uint16_t seconds);
    void standby();
    void wakeup();
};

#endif // GPS_H