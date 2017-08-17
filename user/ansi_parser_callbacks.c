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

// screen manpage - https://www.gnu.org/software/screen/manual/html_node/Control-Sequences.html

static char utf_collect[4];
static int utf_i = 0;
static int utf_j = 0;

/**
 * Handle a received plain character
 */
void ICACHE_FLASH_ATTR
apars_handle_plainchar(char c)
{
	if (c == 7) return; // BELL - beep (TODO play beep in browser)

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
		if (c == 14) {
			// ShiftIN
			screen_set_charset_n(1);
			return;
		}
		if (c == 15) {
			// ShiftOUT
			screen_set_charset_n(0);
			return;
		}

		utf_collect[0] = c;
		utf_collect[1] = 0; // just to make sure it's closed...
		screen_putchar(utf_collect);
	}

	return;
fail:
	ansi_warn("Bad UTF-8");
	apars_reset_utf8buffer();
}

void ICACHE_FLASH_ATTR
apars_reset_utf8buffer(void)
{
	utf_i = 0;
	utf_j = 0;
	memset(utf_collect, 0, 4);
}

void ICACHE_FLASH_ATTR
apars_handle_characterSet(char leadchar, char c)
{
	if (leadchar == '(') screen_set_charset(0, c);
	else if (leadchar == ')') screen_set_charset(1, c);
	// other alternatives * + . - / not implemented
}

void ICACHE_FLASH_ATTR
apars_handle_setXCtrls(char c)
{
	// this does not seem to do anything, sent by some unix programs
//	ansi_warn("NOIMPL Select %cbit ctrls", c=='F'? '7':'8');
}

/**
 * \brief Handle fully received CSI ANSI sequence
 * \param leadchar - private range leading character, 0 if none
 * \param params - array of CSI_N_MAX ints holding the numeric arguments
 * \param keychar - the char terminating the sequence
 */
void ICACHE_FLASH_ATTR
apars_handle_CSI(char leadchar, int *params, int count, char keychar)
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
	}

	switch (keychar) {
		// CUU CUD CUF CUB
		case 'A': screen_cursor_move(-n1, 0, false); break;
		case 'B': screen_cursor_move(n1, 0, false);  break;
		case 'C': screen_cursor_move(0, n1, false);  break;
		case 'D': screen_cursor_move(0, -n1, false); break;

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

			// clear in line
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

		case 'J': // ED - clear screen
			if (n1 == 0) {
				screen_clear(CLEAR_FROM_CURSOR);
			} else if (n1 == 1) {
				screen_clear(CLEAR_TO_CURSOR);
			} else {
				screen_clear(CLEAR_ALL);
				screen_cursor_set(0, 0);
			}
			break;

		case 'K': // EL - clear line
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

		case 'n': // queries
			if (n1 == 6) {
				// Query cursor position
				int x, y;
				screen_cursor_get(&y, &x);
				sprintf(buf, "\033[%d;%dR", y+1, x+1);
				UART_WriteString(UART0, buf, UART_TIMEOUT_US);
			}
			else if (n1 == 5) {
				// Query device status - reply "Device is OK"
				UART_WriteString(UART0, "\033[0n", UART_TIMEOUT_US);
			}
			else {
				ansi_warn("NOIMPL query %d", n1);
			}
			break;

			// DECTCEM feature enable / disable

		case 'h': // feature enable
			yn = 1;
		case 'l': // feature disable
			for (int i = 0; i < count; i++) {
				int n = params[i];
				if (leadchar == '?') {
					if (n == 25) {
						screen_cursor_visible(yn);
					}
					else if (n == 7) {
						screen_wrap_enable(yn);
					}
					else if (n == 1) {
						screen_set_cursors_alt_mode(yn);
					}
					else {
						ansi_warn("NOIMPL DEC opt %d", n);
					}
				}
				else {
					if (n == 4) {
						screen_set_insert_mode(yn);
					}
					else {
						ansi_warn("NOIMPL flag %d", n);
					}
				}
			}
			break;

		case 'm': // SGR - graphics rendition aka attributes
			if (count == 0) {
				count = 1; // this makes it work as 0 (reset)
			}

			// iterate arguments
			for (int i = 0; i < count; i++) {
				int n = params[i];

				if (n == 0) { // reset SGR
					screen_reset_sgr(); // resets colors, inverse and bold.
				}
				else if (n >= 30 && n <= 37) screen_set_fg((Color) (n - 30)); // ANSI normal fg
				else if (n >= 40 && n <= 47) screen_set_bg((Color) (n - 40)); // ANSI normal bg
				else if (n == 39) screen_set_fg(termconf_scratch.default_fg); // default fg
				else if (n == 49) screen_set_bg(termconf_scratch.default_bg); // default bg

				else if (n == 1) screen_attr_enable(ATTR_BOLD);
				else if (n == 2) screen_attr_enable(ATTR_FAINT);
				else if (n == 3) screen_attr_enable(ATTR_ITALIC);
				else if (n == 4) screen_attr_enable(ATTR_UNDERLINE);
				else if (n == 5 || n == 6) screen_attr_enable(ATTR_BLINK); // 6 - rapid blink, not supported
				else if (n == 7) screen_inverse_enable(true);
				else if (n == 9) screen_attr_enable(ATTR_STRIKE);

				else if (n == 20) screen_attr_enable(ATTR_FRAKTUR);
				else if (n == 21) screen_attr_disable(ATTR_BOLD);
				else if (n == 22) screen_attr_disable(ATTR_FAINT);
				else if (n == 23) screen_attr_disable(ATTR_ITALIC|ATTR_FRAKTUR);
				else if (n == 24) screen_attr_disable(ATTR_UNDERLINE);
				else if (n == 25) screen_attr_disable(ATTR_BLINK);
				else if (n == 27) screen_inverse_enable(false);
				else if (n == 29) screen_attr_disable(ATTR_STRIKE);

				else if (n >= 90 && n <= 97) screen_set_fg((Color) (n - 90 + 8)); // AIX bright fg
				else if (n >= 100 && n <= 107) screen_set_bg((Color) (n - 100 + 8)); // AIX bright bg
				else {
					ansi_warn("NOIMPL SGR attr %d", n);
				}
			}
			break;

		case 't': // xterm hacks
			switch(n1) {
				case 8: // set size
					screen_resize(n2, n3);
					break;
				case 18: // report size
					printf(buf, "\033[8;%d;%dt", termconf_scratch.height, termconf_scratch.width);
					UART_WriteString(UART0, buf, UART_TIMEOUT_US);
					break;
				case 11: // Report iconified -> is not iconified
					UART_WriteString(UART0, "\033[1t", UART_TIMEOUT_US);
					break;
				case 21: // Report title
					UART_WriteString(UART0, "\033]L", UART_TIMEOUT_US);
					UART_WriteString(UART0, termconf_scratch.title, UART_TIMEOUT_US);
					UART_WriteString(UART0, "\033\\", UART_TIMEOUT_US);
					break;
				case 24: // Set Height only
					screen_resize(n2, termconf_scratch.width);
					break;
			}
			break;

		case 'L':
			screen_insert_lines(n1);
			break;

		case 'M':
			screen_delete_lines(n1);
			break;

		case '@':
			screen_insert_characters(n1);
			break;

		case 'P':
			screen_delete_characters(n1);
			break;

		case 'r':
			// TODO scrolling region
//			ansi_warn("NOIMPL scrolling region");
			break;

		case 'g':
			// TODO clear tab
			ansi_warn("NOIMPL clear tab");
			break;

		case 'Z':
			// TODO back tab
			ansi_warn("NOIMPL cursor back tab");
			break;

		case 'c': // CSI-c
			// report capabilities (pretend we're vt4xx)
			UART_WriteString(UART0, "\033[?64;22;c", UART_TIMEOUT_US);
			break;

		default:
			ansi_warn("Unknown CSI: %c", keychar);
			apars_handle_badseq();
	}
}

/** codes in the format ESC # n */
void ICACHE_FLASH_ATTR apars_handle_hashCode(char c)
{
	switch(c) {
		case '8':
			screen_fill_with_E();
			break;

		default:
			ansi_warn("Unknown # sequence: %c", c);
	}
}

/** those are single-character escape codes (ESC x) */
void ICACHE_FLASH_ATTR apars_handle_shortCode(char c)
{
	switch(c) {
		case 'c': // screen reset
			screen_reset();
			break;

		case '7': // save cursor + attrs
			screen_cursor_save(true);
			break;

		case '8': // restore cursor + attrs
			screen_cursor_restore(true);
			break;

		case 'E': // same as CR LF
			screen_cursor_move(1, 0, false);
			screen_cursor_set_x(0);
			break;

		case 'D': // move cursor down, scroll screen up if needed
			screen_cursor_move(1, 0, true);
			break;

		case 'M': // move cursor up, scroll screen down if needed
			screen_cursor_move(-1, 0, true);
			break;

		case 'H':
			// TODO set tab
//			ansi_warn("NOIMPL set tab");
			break;

		case '>':
			screen_set_numpad_alt_mode(false);
			break;

		case '<':
			// "Enter ANSI / VT100 mode" - no-op
			break;

		case '=':
			screen_set_numpad_alt_mode(true);
			break;

		default:
			ansi_warn("Unknown 1-char seq %c", c);
	}
}

/**
 * Handle a screen resize request
 */
void ICACHE_FLASH_ATTR
apars_handle_OSC_SetScreenSize(int rows, int cols)
{
	info("OSC: Set screen size to %d x %d", rows, cols);

	screen_resize(rows, cols);
}


void ICACHE_FLASH_ATTR
apars_handle_OSC_SetButton(int num, const char *buffer)
{
	strncpy(termconf_scratch.btn[num-1], buffer, TERM_BTN_LEN);
	info("OSC: Set BTN%d = %s", num, buffer);
	screen_notifyChange(CHANGE_LABELS);
}

void ICACHE_FLASH_ATTR
apars_handle_OSC_SetTitle(const char *buffer)
{
	strncpy(termconf_scratch.title, buffer, TERM_TITLE_LEN);
	info("OSC: Set TITLE = %s", buffer);
	screen_notifyChange(CHANGE_LABELS);
}
