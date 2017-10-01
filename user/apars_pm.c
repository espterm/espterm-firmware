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
#include <httpclient.h>
#include "apars_pm.h"
#include "version.h"
#include "ansi_parser_callbacks.h"
#include "screen.h"
#include "apars_logging.h"
#include "cgi_d2d.h"

/**
 * Helper function to parse incoming DCS (Device Control String)
 * @param msg - the DCS body (after DCS and before ST)
 */
void ICACHE_FLASH_ATTR
apars_handle_pm(char *msg)
{
	if (d2d_parse_command(msg)) return;

	return;
fail:
	ansi_warn("D2D message error: %s", msg);
}
