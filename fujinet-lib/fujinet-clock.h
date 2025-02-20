
/**
 * @brief FujiNet Clock Device Library
 * @license gpl v. 3, see LICENSE for details.
 */

#ifndef FUJINET_CLOCK_H
#define FUJINET_CLOCK_H

#include <stdint.h>

// Read the error codes from network
#include "fujinet-network.h"

// IF ADDITIONAL FORMATS ARE ADDED, DO NOT CHANGE THE CURRENT ORDER OF ENUMS
// UNLESS YOU REFACTOR clock_get_time() FUNCTIONS FOR THE PLATFORMS
// AS THEY RELY ON THE ORDER
typedef enum time_format_t {
	// BINARY formats are just the numbers, not ascii characters for the number.
	SIMPLE_BINARY,      // 7 bytes: [Y(century, e.g. 20), Y(hundreds, e.g. 24), M(1-12), D(1-31), H(0-23), M(0-59), S(0-59)] - Uses the current FN Timezone
	PRODOS_BINARY,      // 4 bytes: special PRODOS format, see https://prodos8.com/docs/techref/adding-routines-to-prodos/ - Uses the current FN Timezone
	APETIME_TZ_BINARY,  // 6 bytes: [Day, Mon, Yr (YY), Hour, Min, Sec] - This version honours the Timezone either set in the WebUI or below in the clock_set_tz (both of which update the FN global timezone value)
	APETIME_BINARY,     // 6 bytes: [Day, Mon, Yr (YY), Hour, Min, Sec] - UTC version of the apetime data. This is for backwards compatibility for Atari Apetime with no TZ set.

	// STRING formats are full null terminated strings
	TZ_ISO_STRING,      // an ISO format: YYYY-MM-DDTHH:MM:SS+HHMM - Uses the current FN Timezone
	UTC_ISO_STRING      // Current UTC time, still ISO format, but with 0000 offset: YYYY-MM-DDTHH:MM:SS+HHMM

} TimeFormat;

/**
 * @brief  Set the FN clock's timezone
 * @param  tz the timezone string to apply
 * @return fujinet-clock status/error code (See FN_ERR_* values)
 */
uint8_t clock_set_tz(const char *tz);

/**
 * @brief  Get the FN clock's timezone
 * @param  tz pointer to the receiving timezone buffer
 * @return fujinet-clock status/error code (See FN_ERR_* values)
 */
uint8_t clock_get_tz(char *tz);

/**
 * @brief  Get the current time in the format specified.
 * @param  time_data pointer to buffer for the response. This is uint8_t, but for STRING formats, will be null terminated and can be treated as a string.
 * @param  format a TimeFormat value to specify how the data should be returned.
 * @return fujinet-clock status/error code (See FN_ERR_* values)
 */
uint8_t clock_get_time(uint8_t* time_data, TimeFormat format);

#define SIO_APETIMECMD_GETTIME 0x93
#define SIO_APETIMECMD_SETTZ 0x99
#define SIO_APETIMECMD_GETTZTIME 0x9A


#endif // FUJINET_CLOCK_H
