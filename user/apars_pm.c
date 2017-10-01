//
// Created by MightyPork on 2017/08/20.
//
// Handle privacy messages
// PM Pt ST
// (PM = ESC ^)
//
// Those are used for device-to-device communication.
// They were not used for anything in the original VT100 and are not
// used by Xterm or any other common emulator, but they should be safely discarded.
//

#include <esp8266.h>
#include "apars_pm.h"
#include "ansi_parser_callbacks.h"
#include "screen.h"
#include "apars_logging.h"

/**
 * Helper function to parse incoming DCS (Device Control String)
 * @param buffer - the DCS body (after DCS and before ST)
 */
void ICACHE_FLASH_ATTR
apars_handle_pm(const char *buffer)
{
	size_t len = strlen(buffer);
	if (false) {
		//
	}
	else {
		ansi_warn("Bad DCS: %s", buffer);
		apars_show_context();
	}
}
