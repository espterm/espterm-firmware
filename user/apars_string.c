//
// Created by MightyPork on 2017/08/20.
//
// String based commands.
// Those command start with a introducer sequence, followed by arbitrary string,
// and end with a String Terminator (`ESC \`, or `BEL`)
//
// ESC k Pt ST ... set screen title (same as `OSC 0 ; Pt ST`, is shorter)
// ESC ] Pt ST ... OSC - Operating System Command (split to its own file)
// ESC P Pt ST ... DCS - Device Control String (split to its own file)
// ESC ^ Pt ST ... PM - Privacy message (unused)
// ESC _ Pt ST ... APC - Application Program Command (unused)
// ESC X Pt ST ... SOS - Start Of String (unused; sent back in response to ENQ)

#include <esp8266.h>
#include "apars_string.h"
#include "apars_logging.h"
#include "ansi_parser_callbacks.h"
#include "screen.h"

// ----- Generic String cmd - disambiguation -----

void ICACHE_FLASH_ATTR
apars_handle_string_cmd(char leadchar, char *buffer)
{
	switch (leadchar) {
		case 'k': // ESC k TITLE ST (defined in GNU screen manpage)
			screen_set_title(buffer);
			break;

		case ']': // OSC - Operating System Command
			apars_handle_osc(buffer);
			break;

		case 'P': // DCS - Device Control String
			apars_handle_dcs(buffer);
			break;

		case '^': // PM - Privacy Message
			break;

		case '_': // APC - Application Program Command
			break;

		case 'X': // SOS - Start of String (purpose unclear)
			break;

		default:
			ansi_warn("Bad str cmd");
			apars_show_context();
	}
}
