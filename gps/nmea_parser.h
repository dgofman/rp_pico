#ifndef NMEA_PARSER_H
#define NMEA_PARSER_H

#include "../lib/uart_pico.h"
#include "../lib/uart_rx.h"
#include "../lib/uart_tx.h"

#define NMEA_PARSER_SUCCESS 0
#define NMEA_PARSER_ERROR_MEMORY_ALLOCATION 1

#ifdef __cplusplus
extern "C"
{
#endif

  // https://receiverhelp.trimble.com/alloy-gnss/en-us/NMEA-0183messages_GGA.html
  typedef struct
  {
    char utc_time[11];     // UTC of position fix (hhmmss.sss)
    double latitude;       // Latitude
    char latitude_dir[2];  // Direction of latitude: (N: North, S: South)
    double longitude;      // Longitude
    char longitude_dir[2]; // Direction of longitude: (E: East, W: West)
    uint8_t fix_status;    // GPS Quality indicator:
    /**
      0: Fix not valid
      1: GPS fix, 2: Differential GPS fix (DGNSS), SBAS, OmniSTAR VBS, Beacon, RTX in GVBS mode
      3: Not applicable
      4: RTK Fixed, xFill
      5: RTK Float, OmniSTAR XP/HP, Location RTK, RTX
      6: INS Dead reckoning
    */
    uint8_t num_satellites; // Number of SVs in use, range from 00 through to 24+
    float hdop;             // Horizontal Dilution of Precision
    float altitude;         // Orthometric height (MSL reference)
    char altitude_unit[2];  // M: unit of measure for orthometric height is meters
    float geoid_separation; // Geoid separation
    char geoid_unit[2];     // M: geoid separation measured in meters
    uint32_t last_time;     // Store the current time (e.g., from a timer)
  } GPGGA_Data;

  // https://receiverhelp.trimble.com/alloy-gnss/en-us/NMEA-0183messages_GLL.html
  typedef struct
  {
    double latitude;       // Latitude in dd mm,mmmm format (0-7 decimal places)
    char latitude_dir[2];  // Direction of latitude N: North S: South
    double longitude;      // Longitude in ddd mm,mmmm format (0-7 decimal places)
    char longitude_dir[2]; // Direction of longitude E: East W: West
    char utc_time[11];     // UTC of position in hhmmss.ss format
    char status[2];        // Status indicator: (A: Data valid, V: Data not valid)
    uint32_t last_time;    // Store the current time (e.g., from a timer)
  } GPGLL_Data;

  // https://receiverhelp.trimble.com/alloy-gnss/en-us/NMEA-0183messages_RMC.html
  typedef struct
  {
    char utc_time[11];     // UTC of position fix
    char status[2];        // Status (A=active or V=void)
    double latitude;       // Latitude in dd mm,mmmm format (0-7 decimal places)
    char latitude_dir[2];  // Direction of latitude N: North S: South
    double longitude;      // Longitude in ddd mm,mmmm format (0-7 decimal places)
    char longitude_dir[2]; // Direction of longitude E: East W: West
    float speed;           // Speed over the ground in knots
    float track;           // Track angle in degrees (True)
    char date[7];          // Date
    float variation;       // Magnetic variation, in degrees
    uint32_t last_time;    // Store the current time (e.g., from a timer)
  } GPRMC_Data;

  // https://receiverhelp.trimble.com/alloy-gnss/en-us/NMEA-0183messages_GSA.html
  typedef struct
  {
    char mode1[2];      // Mode 1: (M = Manual, A = Automatic)
    uint8_t mode2;      // Mode 2: Fix type: (1 = not available, 2 = 2D, 3 = 3D)
    uint8_t prn;        // PRN number: (01 to 32 for GPS, 33 to 64 for SBAS, 64+ for GLONASS)
    uint8_t pdop;       // PDOP: 0.5 to 99.9
    uint8_t hdop;       // HDOP: 0.5 to 99.9
    uint8_t vdop;       // VDOP: 0.5 to 99.9
    uint32_t last_time; // Store the current time (e.g., from a timer)
  } GPGSA_Data;

  // https://receiverhelp.trimble.com/alloy-gnss/en-us/NMEA-0183messages_VTG.html
  typedef struct
  {
    float track1;       // Track made good (degrees true)
    char track1_id[2];  // T: track made good is relative to true north
    float track2;       // Track made good (degrees magnetic)
    char track2_id[2];  // M: track made good is relative to magnetic north
    float speed1;       // Speed, in knots
    char speed1_id[2];  // N: speed is measured in knots
    float speed2;       // Speed over ground in kilometers/hour (kph)
    char speed2_id[2];  // K: speed over ground is measured in kph
    uint32_t last_time; // Store the current time (e.g., from a timer)
  } GPVTG_Data;

  // https://receiverhelp.trimble.com/alloy-gnss/en-us/NMEA-0183messages_GSV.html
  typedef struct
  {
    uint8_t total;      // Total number of messages of this type in this cycle
    uint8_t count;      // Message number
    uint8_t total_sv;   // Total number of SVs visible
    uint8_t prn_sv;     // SV PRN number
    uint8_t elevation;  // Elevation, in degrees, 90° maximum
    uint8_t azimuth;    // Azimuth, degrees from True North, 000° through 359°
    uint8_t snr;        // SNR, 00 through 99 dB (null when not tracking)
    uint32_t last_time; // Store the current time (e.g., from a timer)
  } GPGSV_Data;

  // https://receiverhelp.trimble.com/alloy-gnss/en-us/NMEA-0183messages_MessageOverview.html
  typedef struct
  {
    GPGGA_Data gpgga; // GPGGA - Time, position, and fix related data
    GPGLL_Data gpgll; // GPGLL - Position data: position fix, time of position fix, and status
    GPRMC_Data gprmc; // GPRMC - Position, velocity, and time
    GPGSA_Data gpgsa; // GPGSA - GPS DOP and active satellites
    GPVTG_Data gpvtg; // GPVTG - Track made good and speed over ground
    GPGSV_Data gpgsv; // GPGSV - Satellite information
  } GPSData;

  typedef struct
  {
    UartPico *pico;
    UartRx *uart_rx;
    UartTx *uart_tx;
    GPSData data;

    bool GPGGA_ENABLED; // Flag to enable/disable GPGGA parsing
    bool GPGLL_ENABLED; // Flag to enable/disable GPGLL parsing
    bool GPRMC_ENABLED; // Flag to enable/disable GPRMC parsing
    bool GPGSA_ENABLED; // Flag to enable/disable GPGSA parsing
    bool GPVTG_ENABLED; // Flag to enable/disable GPVTG parsing
    bool GPGSV_ENABLED; // Flag to enable/disable GPGSV parsing
  } NMEAParser;

  int NMEAParser_init(NMEAParser *parser, int rx, int tx);
  int NMEAParser_available(NMEAParser *parser);
  void NMEAParser_sentence(NMEAParser *parser, char *sentence);
  char *NMEAParser_read(NMEAParser *parser);
  void NMEAParser_free(NMEAParser *parser);

#ifdef __cplusplus
}
#endif

#endif // NMEA_PARSER_H