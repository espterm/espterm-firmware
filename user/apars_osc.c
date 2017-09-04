//
// Created by MightyPork on 2017/08/20.
//
// Handle OSC commands (Operating System Command)
// ESC ] Ps ; Pt ST
//
// Those are used to pass various text variables to the terminal:
//
// Ps = 0 or 2 ... set screen title to Pt
// Ps = 81-85  ... set button label to Pt
//

#include "apars_osc.h"
#include "apars_logging.h"
#include "screen.h"
#include "ansi_parser.h"

/**
 * Helper function to parse incoming OSC (Operating System Control)
 * @param buffer - the OSC body (after OSC and before ST)
 */
void ICACHE_FLASH_ATTR
apars_handle_osc(const char *buffer)
{
	const char *orig_buff = buffer;
	int n = 0;
	char c = 0;
	while ((c = *buffer++) != 0) {
		if (c >= '0' && c <= '9') {
			n = (n * 10 + (c - '0'));
		} else {
			break;
		}
	}

	if (c == ';') {
		// Do something with the data string and number
		// (based on xterm manpage)
		if (n == 0 || n == 2) {
			screen_set_title(buffer);
		}
		else if (n >= 81 && n <= 85) { // numbers chosen to not collide with any xterm supported codes
			screen_set_button_text(n - 80, buffer);
		}
		else {
			ansi_noimpl("OSC %d ; %s ST", n, buffer);
		}
	}
	else {
		ansi_warn("BAD OSC: %s", orig_buff);
		apars_show_context();
	}
}
