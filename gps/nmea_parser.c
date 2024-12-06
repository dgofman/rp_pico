#include "nmea_parser.h"
#include "pico/stdlib.h"

#define NMEA_PARSER_SUCCESS 0
#define NMEA_PARSER_ERROR_MEMORY_ALLOCATION 1

/**
 * @brief Initializes the NMEAParser structure with required UART communication settings.
 *
 * This function configures the UART receiver and transmitter, and sets the flags to enable parsing
 * of specific NMEA sentence types (e.g., GPGGA, GPGLL, etc.). If the `UartPico` structure is not already
 * initialized, memory is dynamically allocated for it.
 *
 * @param parser Pointer to the NMEAParser structure to initialize.
 * @param rx The RX pin number for UART communication.
 * @param tx The TX pin number for UART communication.
 *
 * @return Returns `NMEA_PARSER_SUCCESS` (0) on success, or `NMEA_PARSER_ERROR_MEMORY_ALLOCATION` (1) if memory allocation fails.
 */
int NMEAParser_init(NMEAParser *parser, int rx, int tx)
{
  parser->GPGGA_ENABLED = true;
  parser->GPGLL_ENABLED = true;
  parser->GPRMC_ENABLED = true;
  parser->GPGSA_ENABLED = true;
  parser->GPVTG_ENABLED = true;
  parser->GPGSV_ENABLED = true;

  // Initialize UartPico dynamically
  if (parser->pico == NULL)
  {
    parser->pico = (UartPico *)malloc(sizeof(UartPico));
    if (parser->pico == NULL)
    {
      return NMEA_PARSER_ERROR_MEMORY_ALLOCATION; // Return error code
    }
    UartPico uart_pico = UART_PICO();
    *parser->pico = uart_pico;
  }

  // Initialize UART Tx if TX pin is provided
  if (tx > 0)
  {
    parser->uart_tx = UartTx_init(parser->pico, tx);
    UartTx_activate(parser->uart_tx);
  }

  // Initialize UART Rx if RX pin is provided
  if (rx > 0)
  {
    parser->uart_rx = UartRx_init(parser->pico, rx);
    UartRx_activate(parser->uart_rx);
  }

  return NMEA_PARSER_SUCCESS;
}

/**
 * @brief Frees the dynamically allocated memory and deactivates the UART communication channels.
 *
 * This function deallocates memory for `UartPico`, deactivates the UART receiver and transmitter,
 * and cleans up any resources used by the `NMEAParser`.
 *
 * @param parser Pointer to the NMEAParser structure to free.
 */
void NMEAParser_free(NMEAParser *parser)
{
  if (parser->uart_tx != NULL)
  {
    UartTx_free(parser->uart_tx);
  }

  if (parser->uart_rx != NULL)
  {
    UartRx_free(parser->uart_rx);
  }

  if (parser->pico != NULL)
  {
    free(parser->pico); // Free dynamically allocated memory for UartPico
    parser->pico = NULL;
  }
}

/**
 * @brief Checks if there is available data to be read from the UART receiver.
 *
 * This function calls `UartRx_available()` to determine if new data is available for processing.
 *
 * @param parser Pointer to the NMEAParser structure.
 *
 * @return Returns the result from `UartRx_available()`, which indicates the number of available bytes in the UART receiver.
 */
int NMEAParser_available(NMEAParser *parser)
{
  return UartRx_available(parser->uart_rx);
}

/**
 * @brief Returns the current system time in milliseconds since the program started.
 *
 * This function provides the time in milliseconds by dividing the microseconds time (`time_us_32()`) by 1000.
 *
 * @return Returns the current system time in milliseconds.
 */
uint32_t _millis()
{
  return time_us_32() / 1000;
}

/**
 * @brief Extracts a token from the comma-separated string and updates the pointer to the remaining string.
 *
 * This function locates the next comma in the string, null-terminates the token, and updates the
 * `rest` pointer to the position after the comma. If no comma is found, it returns the full string.
 *
 * @param rest Pointer to the string to tokenize. On output, points to the remaining string after the token.
 *
 * @return Returns the extracted token as a string.
 */
static char *getToken(char **rest)
{
  char *start = *rest;
  char *token;
  if (start == NULL || *start == '\0')
  {
    return NULL;
  }

  token = strchr(start, ','); // Find the position of the next comma (or end of string)

  if (token) // If a comma is found, null-terminate the token and move the rest pointer
  {
    *token = '\0';     // Null-terminate the token
    *rest = token + 1; // Move the rest pointer to the character after the comma
  }
  else
  {
    *rest = NULL; // If no more commas, rest becomes NULL (end of string)
  }
  return start;
}

/**
 * @brief Parses the NMEA sentence and updates the parser with the extracted data.
 *
 * This function processes a comma-separated NMEA sentence, extracting relevant tokens and storing
 * them into the appropriate fields in the `NMEAParser` structure. It supports multiple types of NMEA sentences,
 * such as GPGGA, GPGLL, GPRMC, etc., and updates the respective data structures with the parsed values.
 *
 * @param parser Pointer to the NMEAParser structure.
 * @param rest The comma-separated string containing the NMEA sentence to be parsed.
 */
void NMEAParser_rest(NMEAParser *parser, char *rest)
{
  char *token = getToken(&rest); // Tokenize the rest by commas
  if (token == NULL)
  {
    return;
  }

  // Check rest type and parse accordingly
  if (parser->GPGGA_ENABLED && strcmp(token, "$GPGGA") == 0)
  {
    GPGGA_Data *gpgga = &parser->data.gpgga;
    strcpy(gpgga->utc_time, getToken(&rest));        // UTC of position fix
    gpgga->latitude = atof(getToken(&rest));         // Latitude
    strcpy(gpgga->latitude_dir, getToken(&rest));    // Direction of latitude: (N: North, S: South)
    gpgga->longitude = atof(getToken(&rest));        // Longitude
    strcpy(gpgga->longitude_dir, getToken(&rest));   // Direction of longitude: (E: East, W: West)
    gpgga->fix_status = atoi(getToken(&rest));       // GPS Quality indicator
    gpgga->num_satellites = atoi(getToken(&rest));   // Number of SVs in use, range from 00 through to 24+
    gpgga->hdop = atof(getToken(&rest));             // Horizontal Dilution of Precision
    gpgga->altitude = atof(getToken(&rest));         // Orthometric height (MSL reference)
    strcpy(gpgga->altitude_unit, getToken(&rest));   // M: unit of measure for orthometric height is meters
    gpgga->geoid_separation = atof(getToken(&rest)); // Geoid separation
    strcpy(gpgga->geoid_unit, getToken(&rest));      // M: geoid separation measured in meters
    gpgga->last_time = _millis();                    // Store the current time (e.g., from a timer)
  }
  else if (parser->GPGLL_ENABLED && strcmp(token, "$GPGLL") == 0)
  {
    GPGLL_Data *gpgll = &parser->data.gpgll;
    gpgll->latitude = atof(getToken(&rest));       // Latitude in dd mm,mmmm format (0-7 decimal places)
    strcpy(gpgll->latitude_dir, getToken(&rest));  // Direction of latitude N: North S: South
    gpgll->longitude = atof(getToken(&rest));      // Longitude in ddd mm,mmmm format (0-7 decimal places)
    strcpy(gpgll->longitude_dir, getToken(&rest)); // Direction of longitude E: East W: West
    strcpy(gpgll->utc_time, getToken(&rest));      // UTC of position in hhmmss.ss format
    strcpy(gpgll->status, getToken(&rest));        // Status indicator: (A: Data valid, V: Data not valid)
    gpgll->last_time = _millis();                  // Store the current time (e.g., from a timer)
  }
  else if (parser->GPRMC_ENABLED && strcmp(token, "$GPRMC") == 0)
  {
    GPRMC_Data *gprmc = &parser->data.gprmc;
    strcpy(gprmc->utc_time, getToken(&rest));      // UTC of position fix
    strcpy(gprmc->status, getToken(&rest));        // Status (A=active or V=void)
    gprmc->latitude = atof(getToken(&rest));       // Latitude in dd mm,mmmm format (0-7 decimal places)
    strcpy(gprmc->latitude_dir, getToken(&rest));  // Direction of latitude N: North S: South
    gprmc->longitude = atof(getToken(&rest));      // Longitude in ddd mm,mmmm format (0-7 decimal places)
    strcpy(gprmc->longitude_dir, getToken(&rest)); // Direction of longitude E: East W: West
    gprmc->speed = atof(getToken(&rest));          // Speed over the ground in knots
    gprmc->track = atof(getToken(&rest));          // Track angle in degrees (True)
    strcpy(gprmc->date, getToken(&rest));          // Date
    gprmc->variation = atof(getToken(&rest));      // Magnetic variation, in degrees
    gprmc->last_time = _millis();                  // Store the current time (e.g., from a timer)
  }
  else if (parser->GPVTG_ENABLED && strcmp(token, "$GPVTG") == 0)
  {
    GPVTG_Data *gpvtg = &parser->data.gpvtg;
    gpvtg->track1 = atof(getToken(&rest));     // Track made good (degrees true)
    strcpy(gpvtg->track1_id, getToken(&rest)); // T: track made good is relative to true north
    gpvtg->track2 = atof(getToken(&rest));     // Track made good (degrees magnetic)
    strcpy(gpvtg->track2_id, getToken(&rest)); // M: track made good is relative to magnetic north
    gpvtg->speed1 = atof(getToken(&rest));     // Speed, in knots
    strcpy(gpvtg->speed1_id, getToken(&rest)); // N: speed is measured in knots
    gpvtg->speed2 = atof(getToken(&rest));     // Speed over ground in kilometers/hour (kph)
    strcpy(gpvtg->speed2_id, getToken(&rest)); // K: speed over ground is measured in kph
    gpvtg->last_time = _millis();              // Store the current time (e.g., from a timer)
  }
  else if (parser->GPGSV_ENABLED && strcmp(token, "$GPGSV") == 0)
  {
    GPGSV_Data *gpgsv = &parser->data.gpgsv;
    gpgsv->total = atoi(getToken(&rest));     // Total number of messages of this type in this cycle
    gpgsv->count = atoi(getToken(&rest));     // Message number
    gpgsv->total_sv = atoi(getToken(&rest));  // Total number of SVs visible
    gpgsv->prn_sv = atoi(getToken(&rest));    // SV PRN number
    gpgsv->elevation = atoi(getToken(&rest)); // Elevation, in degrees, 90° maximum
    gpgsv->azimuth = atoi(getToken(&rest));   // Azimuth, degrees from True North, 000° through 359°
    gpgsv->snr = atoi(getToken(&rest));       // SNR, 00 through 99 dB (null when not tracking)
    gpgsv->last_time = _millis();             // Store the current time (e.g., from a timer)
  }
}

/**
 * @brief Reads a line of data from the UART receiver and processes the NMEA sentence.
 *
 * This function reads a complete line of data from the UART receiver, tokenizes it, and processes it.
 * It stores the result in the parser's data structure and frees any allocated memory for temporary storage.
 *
 * @param parser Pointer to the NMEAParser structure.
 *
 * @return Returns the read data as a string, or `NULL` if no data is available.
 */
char *NMEAParser_read(NMEAParser *parser)
{
  // Read the next available data (single character)
  char *result = UartRx_readLine(parser->uart_rx);
  if (result == NULL)
  {
    return NULL;
  }
  char *copy = malloc(strlen(result) + 1);
  if (copy != NULL)
  {
    strcpy(copy, result);
    NMEAParser_rest(parser, copy);
    free(copy);
  }
  return result;
}
