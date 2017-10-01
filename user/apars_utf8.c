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
#include "ascii.h"

static u8 bytes[4];
static int utf_len = 0;
static int utf_j = 0;

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

static void ICACHE_FLASH_ATTR screen_print_ascii(const char *str)
{
	char gly[2];
	gly[1] = 0;
	for(int j = 0;str[j]!=0;j++) {
		gly[0] = str[j];
		screen_putchar(gly);
	}
}

static void ICACHE_FLASH_ATTR hdump_spaces_eol(int needed)
{
	if (needed == 0) needed = 5;
	int x, y;
	screen_cursor_get(&y, &x);
	if (x > termconf_live.width - needed) {
		screen_clear_in_line(CLEAR_FROM_CURSOR);
		screen_putchar("\n");
		screen_putchar("\r");
	}
}


static void ICACHE_FLASH_ATTR hdump_good(const char *ch)
{
	char buf[10];
	hdump_spaces_eol(6);

	screen_set_fg(7);
	screen_set_bg(0);
	if(ch[0]<32) {
		screen_set_fg(7);
		screen_set_bg(2);
		switch (ch[0]) {
			case NUL: screen_print_ascii("NUL"); break;
			case SOH: screen_print_ascii("SOH"); break;
			case STX: screen_print_ascii("STX"); break;
			case ETX: screen_print_ascii("ETX"); break;
			case EOT: screen_print_ascii("EOT"); break;
			case ENQ: screen_print_ascii("ENQ"); break;
			case ACK: screen_print_ascii("ACK"); break;
			case BEL: screen_print_ascii("BEL"); break;
			case BS: screen_print_ascii("BS"); break;
			case TAB: screen_print_ascii("TAB"); break;
			case LF: screen_print_ascii("LF"); break;
			case VT: screen_print_ascii("VT"); break;
			case FF: screen_print_ascii("FF"); break;
			case CR: screen_print_ascii("CR"); break;
			case SO: screen_print_ascii("SO"); break;
			case SI: screen_print_ascii("SI"); break;
			case DLE: screen_print_ascii("DLE"); break;
			case DC1: screen_print_ascii("DC1"); break;
			case DC2: screen_print_ascii("DC2"); break;
			case DC3: screen_print_ascii("DC3"); break;
			case DC4: screen_print_ascii("DC4"); break;
			case NAK: screen_print_ascii("NAK"); break;
			case SYN: screen_print_ascii("SYN"); break;
			case ETB: screen_print_ascii("ETB"); break;
			case CAN: screen_print_ascii("CAN"); break;
			case EM: screen_print_ascii("EM"); break;
			case SUB: screen_print_ascii("SUB"); break;
			case ESC: screen_print_ascii("ESC"); break;
			case FS: screen_print_ascii("FS"); break;
			case GS: screen_print_ascii("GS"); break;
			case RS: screen_print_ascii("RS"); break;
			case US: screen_print_ascii("US"); break;
			case SP: screen_print_ascii("SP"); break;
			case DEL: screen_print_ascii("DEL"); break;
			default:
				sprintf(buf, "%02Xh", ch[0]);
				screen_print_ascii(buf);
		}
	} else {
		screen_putchar(ch);
	}

	screen_set_default_bg();
	screen_set_default_fg();
	screen_print_ascii(" ");
}

static void ICACHE_FLASH_ATTR hdump_bad(const char *ch, int len)
{
	char buf[10];
	hdump_spaces_eol(len*5);

	screen_set_fg(7);
	screen_set_bg(1);
	for (int i=0;i<len;i++) {
		sprintf(buf, "%02Xh", ch[i]);
		screen_print_ascii(buf);
		if(i<len-1) screen_print_ascii(" ");
	}
	screen_set_default_bg();
	screen_set_default_fg();
	screen_print_ascii(" ");
}


/**
 * Handle a received plain character
 * @param c - received character
 */
void ICACHE_FLASH_ATTR
apars_handle_plainchar(char c)
{
	u8 uc = (u8)c;
	// collecting unicode glyphs...
	if (uc & 0x80) {
		if (utf_len == 0) {
			bytes[0] = uc;
			utf_j = 1;

			// start
			if (uc == 0xC0 || uc == 0xC1 || uc > 0xF4) {
				// forbidden start codes
				goto fail;
			}

			if ((uc & 0xE0) == 0xC0) {
				utf_len = 2;
			}
			else if ((uc & 0xF0) == 0xE0) {
				utf_len = 3;
			}
			else if ((uc & 0xF8) == 0xF0) {
				utf_len = 4;
			}
			else {
				// chars over 127 that don't start unicode sequences
				goto fail;
			}
		}
		else {
			if ((uc & 0xC0) != 0x80) {
				bytes[utf_j++] = uc;
				goto fail;
			}
			else {
				bytes[utf_j++] = uc;
				if (utf_j >= utf_len) {
					// check for bad sequences - overlong or some other problem
					if (bytes[0] == 0xF4 && bytes[1] > 0x8F) goto fail;
					if (bytes[0] == 0xF0 && bytes[1] < 0x90) goto fail;
					if (bytes[0] == 0xED && bytes[1] > 0x9F) goto fail;
					if (bytes[0] == 0xE0 && bytes[1] < 0xA0) goto fail;

					// trap for surrogates - those break javascript
					if (bytes[0] == 0xED && bytes[1] >= 0xA0 && bytes[1] <= 0xBF) goto fail;

					if (termconf_live.ascii_debug) {
						hdump_good((const char *) bytes);
					} else {
						screen_putchar((const char *) bytes);
					}
					apars_reset_utf8buffer();
				}
			}
		}
	}
	else {
		bytes[0] = uc;
		bytes[1] = 0; // just to make sure it's closed...
		if (termconf_live.ascii_debug) {
			hdump_good((const char *) bytes);
		} else {
			screen_putchar((const char *) bytes);
		}
		apars_reset_utf8buffer();
	}

	return;
fail:
	hdump_bad((const char *) bytes, utf_j);
	ansi_warn("BAD UTF8!");
	apars_show_context();
	apars_reset_utf8buffer();
}
