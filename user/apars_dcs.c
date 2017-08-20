//
// Created by MightyPork on 2017/08/20.
//
// Handle DCS sequences (Device Control String)
// DCS Pt ST
// (DCS = ESC P)
//
// Those are used for terminal status queries.
//
// Pt =
// $ q " p .... read conformance level
// $ q " q .... read character protection attribute
// $ q r ...... read scrolling region extents
// $ q s ...... read horizontal margins extents
// $ q m ...... read SGR in a format that would restore them exactly when run as a CSI Pm m
// $ q SP q ... read cursor style
//
// For details, see
// http://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h2-Device-Control-functions
//

#include <esp8266.h>
#include "apars_dcs.h"
#include "ansi_parser_callbacks.h"
#include "screen.h"
#include "apars_logging.h"

/**
 * Helper function to parse incoming DCS (Device Control String)
 * @param buffer - the DCS body (after DCS and before ST)
 */
void ICACHE_FLASH_ATTR
apars_handle_dcs(const char *buffer)
{
	char buf[64]; // just about big enough for full-house SGR
	size_t len = strlen(buffer);
	if ((len == 3 || len == 4) && strneq(buffer, "$q", 2)) {
		// DECRQSS - Request Status String
		if (strneq(buffer+2, "\"p", 2)) {
			// DECSCL - Select Conformance Level
			apars_respond("\033P1$r64;1\"p\033\\"); // 64;1 - Pretend we are VT400 with 7-bit characters
		}
		else if (strneq(buffer+2, "\"q", 2)) {
			// DECSCA - Select character protection attribute
			sprintf(buf, "\033P1$r%d\"q\033\\", 0); // 0 - Can erase - TODO real protection status if implemented
			apars_respond(buf);
		}
		else if (buffer[2] == 'r') {
			// DECSTBM - Query scrolling region
			sprintf(buf, "\033P1$r%d;%dr\033\\", 1, termconf_scratch.height); // 1-80 TODO real extent of scrolling region
			apars_respond(buf);
		}
		else if (buffer[2] == 's') {
			// DECSLRM - Query horizontal margins
			sprintf(buf, "\033P1$r%d;%ds\033\\", 1, termconf_scratch.width); // Can erase - TODO real extent of horiz margins if implemented
			apars_respond(buf);
		}
		else if (buffer[2] == 'm') {
			// SGR - query SGR
			apars_respond("\033P1$r");
			screen_report_sgr(buf);
			apars_respond(buf);
			apars_respond("m\033\\");
		}
		else if (strneq(buffer+2, " q", 2)) {
			// DECSCUSR - Query cursor style
			sprintf(buf, "\033P1$r%d q\033\\", 1);
			/*
				Ps = 0  -> blinking block.
				Ps = 1  -> blinking block (default).
				Ps = 2  -> steady block.
				Ps = 3  -> blinking underline.
				Ps = 4  -> steady underline.
				Ps = 5  -> blinking bar (xterm).
				Ps = 6  -> steady bar (xterm).
			 */
			apars_respond(buf);
		}
		else {
			// invalid query
			ansi_noimpl("DCS %s ST", buffer);
			sprintf(buf, "\033P0$r%s\033\\", buffer+2);
		}
	}
	else {
		ansi_warn("Bad DCS: %s", buffer);
		apars_show_context();
	}
}
