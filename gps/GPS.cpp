#include "GPS.h"
#include "../lib/uart_rx.h"
#include "../lib/uart_tx.h"
#include <cstdio>
#include <math.h>

/**
 * @brief Updates the NMEA sentence output intervals for the GPS module.
 *
 * This function constructs a command string to set the output intervals for different
 * NMEA sentences (GLL, RMC, VTG, GGA, GSA, GSV) based on the current configuration
 * stored in the `intervals` member of the `GPS` class. The command is sent to the GPS
 * module, enabling or disabling the sentences according to the provided `enable` flag.
 *
 * @param enable - A boolean value indicating whether to enable or disable the intervals.
 *                 If `true`, the intervals are enabled; if `false`, the intervals are disabled.
 */
void GPS::_doUpdateIntervals(bool enable)
{
    // Constructs and sends the NMEA command for updating sentence intervals
    char command[100];
    snprintf(command, sizeof(command),
             "$PMTK314,%d,%d,%d,%d,%d,%d,0,0,0,0,0,0,0,0,0,0,0,0,0*%s",
             intervals.GLL ? 1 : 0,
             intervals.RMC ? 1 : 0,
             intervals.VTG ? 1 : 0,
             intervals.GGA ? 1 : 0,
             intervals.GSA ? 1 : 0,
             intervals.GSV ? 1 : 0,
             enable ? "28" : "29"); // Command format with checksum (enable/disable)
    write(command);
}

/**
 * @brief Converts a latitude or longitude in DDMM format to decimal degrees.
 *
 * This function takes a coordinate in DDMM format (degrees and minutes) and converts it
 * to decimal degrees, which is commonly used in GPS systems. The conversion is done by
 * extracting the degrees and minutes, then calculating the fractional degree value and
 * adding it to the degrees.
 *
 * @param ddmm - The coordinate in DDMM format (degrees and minutes, e.g., 12345.67 for 123° 45.67').
 *
 * @return A double representing the coordinate in decimal degrees.
 *         The result can be positive or negative depending on the direction (e.g., North/South for latitude or East/West for longitude).
 */
double GPS::_convertToDecimalDegrees(double ddmm)
{
    // Converts DDMM format to decimal degrees
    int degrees = (int)(ddmm / 100);             // Extract degrees
    double lat_mm_mmmm = ddmm - (degrees * 100); // Extract minutes
    int minutes = (int)lat_mm_mmmm;
    double fractionalMinutes = lat_mm_mmmm - minutes;

    double minutesDecimal = (minutes + fractionalMinutes) / 60.0; // Convert minutes to decimal
    double decimalDegrees = degrees + minutesDecimal;             // Add degrees and minutes
    return decimalDegrees;
}

/**
 * @brief Constructs a GPS object with the specified sentence intervals.
 * @param intervals - The configuration for sentence intervals.
 */
GPS::GPS(IntervalType intervals)
{
    // Initialize GPS with sentence intervals configuration
    this->intervals = intervals;
    this->start_year = 2000;
}

/**
 * @brief Destructor that frees allocated resources.
 */
GPS::~GPS()
{
    // Clean up allocated resources
    NMEAParser_free(this->_nmeaParser);
    delete this->_nmeaParser;
}

/**
 * @brief Initializes the GPS module with specified RX and TX pins.
 * @param rx - The RX pin number.
 * @param tx - The TX pin number.
 */
void GPS::init(int rx, int tx)
{
    // Initialize NMEA parser with specified RX and TX pins
    this->_nmeaParser = new NMEAParser();
    NMEAParser_init(_nmeaParser, rx, tx);
    updateIntervals();
}

/**
 * @brief Checks if the GPS data is available.
 * @return true if data is available, false otherwise.
 */
bool GPS::isAvailable()
{
    // Checks if new GPS data is available
    return NMEAParser_available(_nmeaParser) > 0;
}

/**
 * @brief Reads GPS data from the NMEA parser.
 * @return A pointer to the NMEA sentence data.
 */
char *GPS::read()
{
    // Reads the next available NMEA sentence
    return NMEAParser_read(_nmeaParser);
}

/**
 * @brief Sends a command string to the GPS module.
 * @param str - The NMEA command string to send.
 */
void GPS::write(const char *str)
{
    // Sends a command string to the GPS module
    UartTx_println(_nmeaParser->uart_tx, str);
}

/**
 * @brief Retrieves the current GPS data.
 * @return A constant reference to the GPSData object.
 */
const GPSData &GPS::getGPSData() const
{
    // Returns the current parsed GPS data
    return _nmeaParser->data;
}

/**
 * @brief Gets the latitude in decimal degrees.
 * @return The latitude value in decimal degrees.
 */
double GPS::latitude()
{
    // Returns latitude in decimal degrees, considering the hemisphere (N/S)
    GPSData data = getGPSData();
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
 * @return The longitude value in decimal degrees.
 */
double GPS::longitude()
{
    // Returns longitude in decimal degrees, considering the hemisphere (E/W)
    GPSData data = getGPSData();
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
 * @return The current date as a long integer.
 */
const long GPS::getDate()
{
    return atol(getGPSData().gprmc.date);
}

/**
 * @brief Gets the year from the GPS date.
 * @return The year part of the current date.
 */
uint16_t GPS::year()
{
    return (getDate() % 100) + start_year;
}

/**
 * @brief Gets the month from the GPS date.
 * @return The month part of the current date.
 */
uint8_t GPS::month()
{
    return (getDate() / 100) % 100;
}

/**
 * @brief Gets the day from the GPS date.
 * @return The day part of the current date.
 */
uint8_t GPS::day()
{
    return getDate() / 10000;
}

/**
 * @brief Retrieves the ground speed in knots.
 * @return The ground speed in knots.
 */
float GPS::speed()
{
    return getGPSData().speed;
}

/**
 * @brief Updates the NMEA sentence output intervals.
 * @see  3.23. Packet Type: 314 PMTK_API_SET_NMEA_OUTPUT
 */
void GPS::updateIntervals()
{
    // Toggle the NMEA sentence output intervals (enable/disable)
    _doUpdateIntervals(false); // Disable intervals first
    _doUpdateIntervals(true);  // Then enable them
}

/**
 * @brief Sets the GPS position update frequency.
 * @param hz - The desired frequency in Hertz (Hz).
 * @see  3.13. Packet Type: 220 PMTK_SET_POS_FIX
 */
void GPS::setFrequency(double hz)
{
    // Sends a command to set the frequency of position updates (in Hz)
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
    write(command);
}

/**
 * @brief Sets the GPS position fix delay.
 * @param seconds - The delay in seconds (maximum is 10 seconds).
 * @see 3.13. Packet Type: 220 PMTK_SET_POS_FIX
 */
void GPS::setDelay(uint16_t seconds)
{
    // Converts delay in seconds to Hz and sets the frequency
    setFrequency(1.0 / seconds);
}

/**
 * @brief Puts the GPS module in standby mode.
 * @see 3.8. Packet Type: 161 PMTK_CMD_STANDBY_MODE
 */
void GPS::standby()
{
    // Puts GPS into standby mode
    write("$PMTK161,0*28");
}

/**
 * @brief Wakes up the GPS module from standby mode.
 */
void GPS::wakeup()
{
    // Wakes up GPS from standby mode
    write("$PMTK161,0*29");
}
