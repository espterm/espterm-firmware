/**
 * ANSI code parser callbacks that do the actual work.
 * Split from the parser itself for easier editing without
 * having to deal with Ragel.
 */

#include <esp8266.h>
#include "ansi_parser_callbacks.h"
#include "screen.h"
#include "ansi_parser.h"
#include "uart_driver.h"
#include "sgr.h"

static char utf_collect[4];
static int utf_i = 0;
static int utf_j = 0;

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
				// forbidden codes
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
 * Send a response to UART0
 * @param str
 */
static void ICACHE_FLASH_ATTR
respond(const char *str)
{
	UART_WriteString(UART0, str, UART_TIMEOUT_US);
}

/**
 * Command to assign G0 or G1
 * @param leadchar - ( or ) for G0 or G1
 * @param c - character table ID (0, B etc)
 */
void ICACHE_FLASH_ATTR
apars_handle_characterSet(char leadchar, char c)
{
	if (leadchar == '(') screen_set_charset(0, c);
	else if (leadchar == ')') screen_set_charset(1, c);
	else {
		ansi_warn("NOIMPL: ESC %c %c", leadchar, c);
	}
	// other alternatives * + . - / not implemented
}

/**
 * ESC SP <c> (this sets 8/7-bit mode and some other archaic options)
 * @param c - key character
 */
void ICACHE_FLASH_ATTR
apars_handle_spaceCmd(char c)
{
	ansi_warn("NOIMPL: ESC SP %c", c);
}

/**
 * Beep at the user.
 */
void ICACHE_FLASH_ATTR
apars_handle_bel(void)
{
	ansi_warn("NOIMPL: BEEP");
	// TODO pass to the browser somehow
}

/**
 * Handle fully received CSI ANSI sequence
 * @param leadchar - private range leading character, 0 if none
 * @param params - array of CSI_N_MAX ints holding the numeric arguments
 * @param keychar - the char terminating the sequence
 */
void ICACHE_FLASH_ATTR
apars_handle_CSI(char leadchar, const int *params, int count, char keychar)
{
	int n1 = params[0];
	int n2 = params[1];
	int n3 = params[2];
	static char buf[20];
	bool yn = 0; // for ? l h

	// defaults
	switch (keychar) {
		case 'A': // move
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		case 'G': // set X
		case '`':
		case 'S': // scrolling
		case 'T':
		case 'X': // clear in line
		case 'd': // set Y
		case 'L':
		case 'M':
		case '@':
		case 'P':
		case 'I':
		case 'Z':
			if (n1 == 0) n1 = 1;
			break;

		case 'H':
		case 'f':
			if (n1 == 0) n1 = 1;
			if (n2 == 0) n2 = 1;
			break;

		case 'J':
		case 'K':
			if (n1 > 2) n1 = 0;
			break;

		default:
			// leave as is
			break;
	}

	switch (keychar) {
		// CUU CUD CUF CUB
		case 'a':
		case 'A': // Up
			screen_cursor_move(-n1, 0, false);
			break;

		case 'e':
		case 'B': // Down
			screen_cursor_move(n1, 0, false);
			break;

		case 'C': // Right (forward)
			screen_cursor_move(0, n1, false);
			break;

		case 'D': // Left (backward)
			screen_cursor_move(0, -n1, false);
			break;

		case 'E': // CNL - Cursor Next Line
			screen_cursor_move(n1, 0, false);
			screen_cursor_set_x(0);
			break;

		case 'F': // CPL - Cursor Prev Line
			screen_cursor_move(-n1, 0, false);
			screen_cursor_set_x(0);
			break;

			// Set X
		case 'G':
		case '`': // alternate code
			screen_cursor_set_x(n1 - 1);
			break; // 1-based

			// Set Y
		case 'd':
			screen_cursor_set_y(n1 - 1);
			break; // 1-based

			// Clear in line
		case 'X':
			screen_clear_in_line(n1);
			break; // 1-based

			// SU, SD - scroll up/down
		case 'S': screen_scroll_up(n1);	  break;
		case 'T': screen_scroll_down(n1); break;

			// CUP,HVP - set position
		case 'H':
		case 'f':
			screen_cursor_set(n1-1, n2-1);
			break; // 1-based

		case 'J': // Erase screen
			if (n1 == 0) {
				screen_clear(CLEAR_FROM_CURSOR);
			} else if (n1 == 1) {
				screen_clear(CLEAR_TO_CURSOR);
			} else {
				screen_clear(CLEAR_ALL);
				screen_cursor_set(0, 0);
			}
			break;

		case 'K': // Erase lines
			if (n1 == 0) {
				screen_clear_line(CLEAR_FROM_CURSOR);
			} else if (n1 == 1) {
				screen_clear_line(CLEAR_TO_CURSOR);
			} else {
				screen_clear_line(CLEAR_ALL);
			}
			break;

			// SCP, RCP - save/restore position
		case 's': screen_cursor_save(0); break;
		case 'u': screen_cursor_restore(0); break;

		case 'n': // Queries
			if (n1 == 6) {
				// Query cursor position
				int x, y;
				screen_cursor_get(&y, &x);
				sprintf(buf, "\033[%d;%dR", y+1, x+1);
				respond(buf);
			}
			else if (n1 == 5) {
				// Query device status - reply "Device is OK"
				respond("\033[0n");
			}
			else {
				ansi_warn("NOIMPL: CSI %d n", n1);
			}
			break;

		case 'h': // DEC feature enable
			yn = 1;
		case 'l': // DEC feature disable
			for (int i = 0; i < count; i++) {
				int n = params[i];
				if (leadchar == '?') {
					if (n == 1) {
						screen_set_cursors_alt_mode(yn);
					}
					else if (n == 6) {
						// TODO origin mode (scrolling region)
					}
					else if (n == 7) {
						screen_wrap_enable(yn);
					}
					else if (n == 8) {
						// Key auto-repeat
						// We don't implement this currently, but it could be added
						// - discard repeated keypress events between keydown and keyup.
					}
					else if (n == 9 || (n >= 1000 && n <= 1006)) {
						// TODO mouse
						// 1000 - C11 mouse - Send Mouse X & Y on button press and release.
						// 1001 - Hilite mouse tracking
						// 1002 - Cell Motion Mouse Tracking
						// 1003 - All Motion Mouse Tracking
						// 1004 - Send FocusIn/FocusOut events
						// 1005 - Enable UTF-8 Mouse Mode
						// 1006 - SGR mouse mode
					}
					else if (n == 12) {
						// TODO Cursor blink on/off
					}
					else if (n == 1049) {
						// XTERM: optionally switch to/from alternate screen
						// We can't implement this because of limited RAM size
					}
					else if (n == 2004) {
						// Bracketed paste mode
						// Discard, we don't implement this
					}
					else if (n == 25) {
						screen_cursor_visible(yn);
					}
					else {
						ansi_warn("NOIMPL: CSI ? %d %c", n, keychar);
					}
				}
				else {
					if (n == 4) {
						screen_set_insert_mode(yn);
					}
					else if (n == 20) {
						screen_set_newline_mode(yn);
					}
					else {
						ansi_warn("NOIMPL: CSI %d %c", n, keychar);
					}
				}
			}
			break;

		case 'm': // SGR - set graphics rendition
			if (count == 0) {
				count = 1; // this makes it work as 0 (reset)
			}

			// iterate arguments
			for (int i = 0; i < count; i++) {
				int n = params[i];

				if (n == SGR_RESET) screen_reset_sgr();
				// -- set color --
				else if (n >= SGR_FG_START && n <= SGR_FG_END) screen_set_fg((Color) (n - SGR_FG_START)); // ANSI normal fg
				else if (n >= SGR_BG_START && n <= SGR_BG_END) screen_set_bg((Color) (n - SGR_BG_START)); // ANSI normal bg
				else if (n == SGR_FG_DEFAULT) screen_set_fg(termconf_scratch.default_fg); // default fg
				else if (n == SGR_BG_DEFAULT) screen_set_bg(termconf_scratch.default_bg); // default bg
				// -- set attr --
				else if (n == SGR_BOLD) screen_attr_enable(ATTR_BOLD);
				else if (n == SGR_FAINT) screen_attr_enable(ATTR_FAINT);
				else if (n == SGR_ITALIC) screen_attr_enable(ATTR_ITALIC);
				else if (n == SGR_UNDERLINE) screen_attr_enable(ATTR_UNDERLINE);
				else if (n == SGR_BLINK || n == SGR_BLINK_FAST) screen_attr_enable(ATTR_BLINK); // 6 - rapid blink, not supported
				else if (n == SGR_INVERSE) screen_inverse_enable(true);
				else if (n == SGR_STRIKE) screen_attr_enable(ATTR_STRIKE);
				else if (n == SGR_FRAKTUR) screen_attr_enable(ATTR_FRAKTUR);
				// -- clear attr --
				else if (n == SGR_OFF(SGR_BOLD)) screen_attr_disable(ATTR_BOLD);
				else if (n == SGR_OFF(SGR_FAINT)) screen_attr_disable(ATTR_FAINT);
				else if (n == SGR_OFF(SGR_ITALIC)) screen_attr_disable(ATTR_ITALIC|ATTR_FRAKTUR);
				else if (n == SGR_OFF(SGR_UNDERLINE)) screen_attr_disable(ATTR_UNDERLINE);
				else if (n == SGR_OFF(SGR_BLINK)) screen_attr_disable(ATTR_BLINK);
				else if (n == SGR_OFF(SGR_INVERSE)) screen_inverse_enable(false);
				else if (n == SGR_OFF(SGR_STRIKE)) screen_attr_disable(ATTR_STRIKE);
				// -- AIX bright colors --
				else if (n >= SGR_FG_BRT_START && n <= SGR_FG_BRT_END) screen_set_fg((Color) ((n - SGR_FG_BRT_START) + 8)); // AIX bright fg
				else if (n >= SGR_BG_BRT_START && n <= SGR_BG_BRT_END) screen_set_bg((Color) ((n - SGR_BG_BRT_START) + 8)); // AIX bright bg
				else {
					ansi_warn("NOIMPL: SGR %d", n);
				}
			}
			break;

		case 't': // xterm window hacks
			switch(n1) {
				case 8: // set size
					screen_resize(n2, n3);
					break;
				case 18: // report size
					printf(buf, "\033[8;%d;%dt", termconf_scratch.height, termconf_scratch.width);
					respond(buf);
					break;
				case 11: // Report iconified -> is not iconified
					respond("\033[1t");
					break;
				case 21: // Report title
					respond("\033]L");
					respond(termconf_scratch.title);
					respond("\033\\");
					break;
				case 24: // Set Height only
					screen_resize(n2, termconf_scratch.width);
					break;
				default:
					ansi_warn("NOIMPL CSI %d t", n1);
					break;
			}
			break;

		case 'L': // Insert lines (shove down)
			screen_insert_lines(n1);
			break;

		case 'M': // Delete lines (pull up)
			screen_delete_lines(n1);
			break;

		case '@': // Insert in line (shove right)
			screen_insert_characters(n1);
			break;

		case 'P': // Delete in line (pull left)
			screen_delete_characters(n1);
			break;

		case 'r':
			// TODO scrolling region
			break;

		case 'g': // Clear tabs
			if (n1 == 3) {
				screen_clear_all_tabs();
			} else {
				screen_clear_tab();
			}
			break;

		case 'Z': // Tab backward
			for(; n1 > 0; n1--) {
				screen_tab_reverse();
			}
			break;

		case 'I': // Tab forward
			for(; n1 > 0; n1--) {
				screen_tab_forward();
			}
			break;

		case 'c': // CSI-c - report capabilities
			respond("\033[?64;22;c"); // pretend we're vt400
			break;

		default:
			ansi_warn("NOIMPL: CSI Pm %c", keychar);
			apars_handle_badseq();
	}
}

/**
 * Codes in the format ESC # n
 * @param c - the trailing symbol (numeric ASCII)
 */
void ICACHE_FLASH_ATTR apars_handle_hashCode(char c)
{
	switch(c) {
		case '8':
			screen_fill_with_E();
			break;

		default:
			ansi_warn("NOIMPL: ESC # %c", c);
	}
}

/**
 * Single-character escape codes (ESC x)
 * @param c - the trailing symbol (ASCII)
 */
void ICACHE_FLASH_ATTR apars_handle_shortCode(char c)
{
	switch(c) {
		case 'c': // screen reset
			screen_reset();
			break;

		case '7': // save cursor + attributes
			screen_cursor_save(true);
			break;

		case '8': // restore cursor + attributes
			screen_cursor_restore(true);
			break;

		case 'E': // same as CR LF
			screen_cursor_move(1, 0, false);
			screen_cursor_set_x(0);
			break;

		case 'F': // bottom left
			screen_cursor_set(termconf_scratch.height-1, 0);
			break;

		case 'D': // move cursor down, scroll screen up if needed
			screen_cursor_move(1, 0, true);
			break;

		case 'M': // move cursor up, scroll screen down if needed
			screen_cursor_move(-1, 0, true);
			break;

		case 'H':
			screen_set_tab();
			break;

		case '>':
			screen_set_numpad_alt_mode(false);
			break;

		case '<': // "Enter ANSI / VT100 mode" - no-op (we don't support VT52 mode)
			break;

		case '=':
			screen_set_numpad_alt_mode(true);
			break;

		case '|': // Invoke the G3 Character Set as GR (LS3R).
		case '}': // Invoke the G2 Character Set as GR (LS2R).
		case '~': // Invoke the G1 Character Set as GR (LS1R).
			// Those do not seem to do anything TODO investigate
			break;

		case '@': // no-op padding char (?)
			break;

		case '\\': // spurious string terminator
			break;

		default:
			ansi_warn("NOIMPL: ESC %c", c);
	}
}

/**
 * Helper function to set terminal title
 * @param str - title text
 */
static void ICACHE_FLASH_ATTR
set_title(const char *str)
{
	strncpy(termconf_scratch.title, str, TERM_TITLE_LEN);
	screen_notifyChange(CHANGE_LABELS);
}

/**
 * Helper function to set terminal button label
 * @param num - button number 1-5
 * @param str - button text
 */
static void ICACHE_FLASH_ATTR
set_button_text(int num, const char *str)
{
	strncpy(termconf_scratch.btn[num-1], str, TERM_BTN_LEN);
	screen_notifyChange(CHANGE_LABELS);
}

/**
 * Helper function to parse incoming OSC (Operating System Control)
 * @param buffer - the OSC body (after OSC and before ST)
 */
static void ICACHE_FLASH_ATTR
parse_osc(const char *buffer)
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
		if (n == 0 || n == 2) set_title(buffer);
		else if (n >= 81 && n <= 85) { // numbers chosen to not collide with any xterm supported codes
			set_button_text(n - 80, buffer);
		}
		else {
			ansi_warn("NOIMPL: OSC %d ; %s ST", n, buffer);
		}
	}
	else {
		ansi_warn("BAD OSC: %s", orig_buff);
	}
}

/**
 * Helper function to parse incoming DCS (Device Control String)
 * @param buffer - the DCS body (after DCS and before ST)
 */
static void ICACHE_FLASH_ATTR
parse_dcs(const char *buffer)
{
	char buf[64]; // just about big enough for full-house SGR
	size_t len = strlen(buffer);
	if ((len == 3 || len == 4) && strneq(buffer, "$q", 2)) {
		// DECRQSS - Request Status String
		if (strneq(buffer+2, "\"p", 2)) {
			// DECSCL - Select Conformance Level
			respond("\033[P1$r64;1\"p\033\\"); // 64;1 - Pretend we are VT400 with 7-bit characters
		}
		else if (strneq(buffer+2, "\"q", 2)) {
			// DECSCA - Select character protection attribute
			sprintf(buf, "\033[P1$r%d\"q\033\\", 0); // 0 - Can erase - TODO real protection status if implemented
			respond(buf);
		}
		else if (buffer[2] == 'r') {
			// DECSTBM - Query scrolling region
			sprintf(buf, "\033[P1$r%d;%dr\033\\", 1, termconf_scratch.height); // 1-80 TODO real extent of scrolling region
			respond(buf);
		}
		else if (buffer[2] == 's') {
			// DECSLRM - Query horizontal margins
			sprintf(buf, "\033[P1$r%d;%ds\033\\", 1, termconf_scratch.width); // Can erase - TODO real extent of horiz margins if implemented
			respond(buf);
		}
		else if (buffer[2] == 'm') {
			// SGR - query SGR
			respond("\033[P1$r");
			screen_report_sgr(buf);
			respond(buf);
			respond("m\033\\");
		}
		else if (strneq(buffer+2, " q", 2)) {
			// DECSCUSR - Query cursor style
			sprintf(buf, "\033[P1$r%d q\033\\", 1);
			/*
				Ps = 0  -> blinking block.
				Ps = 1  -> blinking block (default).
				Ps = 2  -> steady block.
				Ps = 3  -> blinking underline.
				Ps = 4  -> steady underline.
				Ps = 5  -> blinking bar (xterm).
				Ps = 6  -> steady bar (xterm).
			 */
			respond(buf);
		}
		else {
			// invalid query
			ansi_warn("NOIMPL: DCS %s ST", buffer);
			sprintf(buf, "\033[P0$r%s\033\\", buffer+2);
		}
	}
	else {
		ansi_warn("NOIMPL: DCS %s ST", buffer);
	}
}

void ICACHE_FLASH_ATTR
apars_handle_StrCmd(char leadchar, const char *buffer)
{
	switch (leadchar) {
		case 'k': // ESC k TITLE ST (defined in GNU screen manpage)
			set_title(buffer);
			break;
		case ']': // OSC - Operating System Command
			parse_osc(buffer);
			break;
		case 'P': // DCS - Device Control String
			parse_dcs(buffer);
			break;
		case '^': // PM - Privacy Message
			break;
		case '_': // APC - Application Program Command
			break;
		case 'X': // SOS - Start of String (purpose unclear)
			break;
		default:
			ansi_warn("NOIMPL: ESC %c Pt ST", leadchar);
	}
}
