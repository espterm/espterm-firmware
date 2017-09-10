//
// Created by MightyPork on 2017/08/20.
//
// UTF-8 parser - collects bytes of a code point before writing them
// into a screen cell.
//

#include "apars_utf8.h"
#include "apars_logging.h"
#include "screen.h"

static char utf_collect[4];
static int utf_i = 0;
static int utf_j = 0;

/**
 * Clear the buffer where we collect pieces of a code point.
 * This is used for parser reset.
 */
void ICACHE_FLASH_ATTR
apars_reset_utf8buffer(void)
{
	utf_i = 0;
	utf_j = 0;
	memset(utf_collect, 0, 4);
}

/**
 * Handle a received plain character
 * @param c - received character
 */
void ICACHE_FLASH_ATTR
apars_handle_plainchar(char c)
{
	// collecting unicode glyphs...
	if (c & 0x80) {
		if (utf_i == 0) {
			// start
			if (c == 192 || c == 193 || c >= 245) {
				// forbidden codes (would be an overlong sequence)
				goto fail;
			}

			if ((c & 0xE0) == 0xC0) {
				utf_i = 2;
			}
			else if ((c & 0xF0) == 0xE0) {
				utf_i = 3;
			}
			else if ((c & 0xF8) == 0xF0) {
				utf_i = 4;
			}
			else {
				// chars over 127 that don't start unicode sequences
				goto fail;
			}

			utf_collect[0] = c;
			utf_j = 1;
		}
		else {
			if ((c & 0xC0) != 0x80) {
				goto fail;
			}
			else {
				utf_collect[utf_j++] = c;
				if (utf_j >= utf_i) {
					screen_putchar(utf_collect);
					apars_reset_utf8buffer();
				}
			}
		}
	}
	else {
		utf_collect[0] = c;
		utf_collect[1] = 0; // just to make sure it's closed...
		screen_putchar(utf_collect);
	}

	return;
	fail:
	ansi_warn("Bad UTF-8: %0Xh", c);
	apars_reset_utf8buffer();
}
