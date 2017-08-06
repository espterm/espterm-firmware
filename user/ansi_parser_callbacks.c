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
#include "persist.h"

static char utf_collect[4];
static int utf_i = 0;
static int utf_j = 0;

/**
 * Handle a received plain character
 */
void ICACHE_FLASH_ATTR
apars_handle_plainchar(char c)
{
	// collecting unicode glyphs...
	if (c & 0x80) {
		if (utf_i == 0) {
			if ((c & 0xE0) == 0xC0) {
				utf_i = 2;
			}
			else if ((c & 0xF0) == 0xE0) {
				utf_i = 3;
			}
			else if ((c & 0xF8) == 0xF0) {
				utf_i = 4;
			}

			utf_collect[0] = c;
			utf_j = 1;
		}
		else {
			utf_collect[utf_j++] = c;
			if (utf_j >= utf_i) {
				screen_putchar(utf_collect);
				utf_i = 0;
				utf_j = 0;
				memset(utf_collect, 0, 4);
			}
		}
	}
	else {
		utf_collect[0] = c;
		utf_collect[1] = 0; // just to make sure it's closed...
		screen_putchar(utf_collect);
	}
}

/**
 * Bad sequence received, show warning
 */
void apars_handle_badseq(void)
{
	warn("Invalid escape sequence discarded.");
}

/**
 * \brief Handle fully received CSI ANSI sequence
 * \param leadchar - private range leading character, 0 if none
 * \param params - array of CSI_N_MAX ints holding the numeric arguments
 * \param keychar - the char terminating the sequence
 */
void ICACHE_FLASH_ATTR
apars_handle_CSI(char leadchar, int *params, char keychar)
{
	/*
		Implemented codes (from Wikipedia)

		CSI n A	CUU – Cursor Up
		CSI n B	CUD – Cursor Down
		CSI n C	CUF – Cursor Forward
		CSI n D	CUB – Cursor Back
		CSI n E	CNL – Cursor Next Line
		CSI n F	CPL – Cursor Previous Line
		CSI n G	CHA – Cursor Horizontal Absolute
		CSI n ; m H	CUP – Cursor Position
		CSI n J	ED – Erase Display
		CSI n K	EL – Erase in Line
		CSI n S	SU – Scroll Up
		CSI n T	SD – Scroll Down
		CSI n ; m f	HVP – Horizontal and Vertical Position
		CSI n m	SGR – Select Graphic Rendition  (Implemented only some)
		CSI 6n	DSR – Device Status Report
		CSI s	SCP – Save Cursor Position
		CSI u	RCP – Restore Cursor Position
		CSI ?25l	DECTCEM	Hides the cursor
		CSI ?25h	DECTCEM	Shows the cursor
	*/

	int n1 = params[0];
	int n2 = params[1];
//	int n3 = params[2];

	// defaults
	switch (keychar) {
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		case 'G':
		case 'S':
		case 'T':
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
		case 'A': screen_cursor_move(-n1, 0); break;
		case 'B': screen_cursor_move(n1, 0);  break;
		case 'C': screen_cursor_move(0, n1);  break;
		case 'D': screen_cursor_move(0, -n1); break;

		case 'E': // CNL
			screen_cursor_move(n1, 0);
			screen_cursor_set_x(0);
			break;

		case 'F': // CPL
			screen_cursor_move(-n1, 0);
			screen_cursor_set_x(0);
			break;

			// CHA
		case 'G':
			screen_cursor_set_x(n1 - 1); break; // 1-based

			// SU, SD
		case 'S': screen_scroll_up(n1);	  break;
		case 'T': screen_scroll_down(n1); break;

			// CUP,HVP
		case 'H':
		case 'f':
			screen_cursor_set(n1-1, n2-1); break; // 1-based

		case 'J': // ED
			if (n1 == 0) {
				screen_clear(CLEAR_TO_CURSOR);
			} else if (n1 == 1) {
				screen_clear(CLEAR_FROM_CURSOR);
			} else {
				screen_clear(CLEAR_ALL);
				screen_cursor_set(0, 0);
			}
			break;

		case 'K': // EL
			if (n1 == 0) {
				screen_clear_line(CLEAR_TO_CURSOR);
			} else if (n1 == 1) {
				screen_clear_line(CLEAR_FROM_CURSOR);
			} else {
				screen_clear_line(CLEAR_ALL);
			}
			break;

			// SCP, RCP
		case 's': screen_cursor_save(0); break;
		case 'u': screen_cursor_restore(0); break;

		case 'n':
			if (n1 == 6) {
				// Query cursor position
				char buf[20];
				int x, y;
				screen_cursor_get(&y, &x);
				sprintf(buf, "\033[%d;%dR", y+1, x+1);
				UART_WriteString(UART0, buf, UART_TIMEOUT_US);
			}
			else if (n1 == 5) {
				// Query device status - reply "Device is OK"
				UART_WriteString(UART0, "\033[0n", UART_TIMEOUT_US);
			}
			break;

			// DECTCEM feature enable / disable

		case 'h':
			if (leadchar == '?') {
				if (n1 == 25) {
					screen_cursor_enable(1);
				} else if (n1 == 7) {
					screen_wrap_enable(1);
				}
			}
			break;

		case 'l':
			if (leadchar == '?') {
				if (n1 == 25) {
					screen_cursor_enable(0);
				} else if (n1 == 7) {
					screen_wrap_enable(0);
				}
			}
			break;

		case 'm': // SGR
			// iterate arguments
			for (int i = 0; i < CSI_N_MAX; i++) {
				int n = params[i];

				if (i == 0 && n == 0) { // reset SGR
					screen_reset_cursor(); // resets colors, inverse and bold.
					break; // cannot combine reset with others
				}
				else if (n >= 30 && n <= 37) screen_set_fg(n-30); // ANSI normal fg
				else if (n >= 40 && n <= 47) screen_set_bg(n-40); // ANSI normal bg
				else if (n == 39) screen_set_fg(7); // default fg
				else if (n == 49) screen_set_bg(false); // default bg
				else if (n == 7) screen_inverse(true); // inverse
				else if (n == 27) screen_inverse(false); // positive
				else if (n == 1) screen_set_bold(true); // bold
				else if (n == 21 || n == 22) screen_set_bold(false); // bold off
				else if (n >= 90 && n <= 97) screen_set_fg(n-90+8); // AIX bright fg
				else if (n >= 100 && n <= 107) screen_set_bg(n-100+8); // AIX bright bg
			}
			break;
	}
}

void ICACHE_FLASH_ATTR apars_handle_hashCode(char c)
{
	//
}

void ICACHE_FLASH_ATTR apars_handle_shortCode(char c)
{
	switch(c) {
		case 'c': // screen reset
			screen_reset();
			break;
		case '7': // save cursor + attrs
			screen_cursor_save(1);
			break;
		case '8': // restore cursor + attrs
			screen_cursor_restore(1);
			break;
		case 'E': // same as CR LF
			// TODO
			break;
		case 'D': // move cursor down, scroll screen up if needed
			// TODO
			break;
		case 'M': // move cursor up, scroll screen down if needed
			// TODO
			break;
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
