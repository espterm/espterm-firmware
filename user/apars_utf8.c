//
// Created by MightyPork on 2017/08/20.
//
// UTF-8 parser - collects bytes of a code point before writing them
// into a screen cell.
//

#include "apars_utf8.h"
#include "apars_logging.h"
#include "screen.h"
#include "uart_driver.h"
#include "ansi_parser_callbacks.h"
#include "ansi_parser.h"

static u8 bytes[4];
static int utf_len = 0;
static int utf_j = 0;

ETSTimer timerResumeRx;

void ICACHE_FLASH_ATTR resumeRxCb(void *unused)
{
	ansi_dbg("Parser recover.");
	ansi_parser_inhibit = false;
}

/**
 * Clear the buffer where we collect pieces of a code point.
 * This is used for parser reset.
 */
void ICACHE_FLASH_ATTR
apars_reset_utf8buffer(void)
{
	utf_len = 0;
	utf_j = 0;
	memset(bytes, 0, 4);
}

//      Code Points      First Byte Second Byte Third Byte Fourth Byte
//  U+0000 -   U+007F     00 - 7F
//  U+0080 -   U+07FF     C2 - DF    80 - BF
//	U+0800 -   U+0FFF     E0         *A0 - BF     80 - BF
//	U+1000 -   U+CFFF     E1 - EC    80 - BF     80 - BF
//	U+D000 -   U+D7FF     ED         80 - *9F     80 - BF
//	U+E000 -   U+FFFF     EE - EF    80 - BF     80 - BF
//	U+10000 -  U+3FFFF    F0         *90 - BF     80 - BF    80 - BF
//	U+40000 -  U+FFFFF    F1 - F3    80 - BF     80 - BF    80 - BF
//	U+100000 - U+10FFFF   F4         80 - *8F     80 - BF    80 - BF

/**
 * Handle a received plain character
 * @param c - received character
 */
void ICACHE_FLASH_ATTR
apars_handle_plainchar(char c)
{
	// collecting unicode glyphs...
	if (c & 0x80) {
		if (utf_len == 0) {
			// start
			if (c == 0xC0 || c == 0xC1 || c > 0xF4) {
				// forbidden start codes
				goto fail;
			}

			if ((c & 0xE0) == 0xC0) {
				utf_len = 2;
			}
			else if ((c & 0xF0) == 0xE0) {
				utf_len = 3;
			}
			else if ((c & 0xF8) == 0xF0) {
				utf_len = 4;
			}
			else {
				// chars over 127 that don't start unicode sequences
				goto fail;
			}

			bytes[0] = c;
			utf_j = 1;
		}
		else {
			if ((c & 0xC0) != 0x80) {
				goto fail;
			}
			else {
				bytes[utf_j++] = c;
				if (utf_j >= utf_len) {
					// check for bad sequences - overlong or some other problem
					if (bytes[0] == 0xF4 && bytes[1] > 0x8F) goto fail;
					if (bytes[0] == 0xF0 && bytes[1] < 0x90) goto fail;
					if (bytes[0] == 0xED && bytes[1] > 0x9F) goto fail;
					if (bytes[0] == 0xE0 && bytes[1] < 0xA0) goto fail;

					// trap for surrogates - those break javascript
					if (bytes[0] == 0xED && bytes[1] >= 0xA0 && bytes[1] <= 0xBF) goto fail;

					screen_putchar((const char *) bytes);
					apars_reset_utf8buffer();
				}
			}
		}
	}
	else {
		bytes[0] = c;
		bytes[1] = 0; // just to make sure it's closed...
		screen_putchar((const char *) bytes);
	}

	return;
fail:
	ansi_parser_inhibit = true;

	ansi_warn("BAD UTF8!");
	apars_show_context();
	apars_reset_utf8buffer();
	ansi_dbg("Temporarily inhibiting parser...");
	TIMER_START(&timerResumeRx, resumeRxCb, 500, 0);
}
