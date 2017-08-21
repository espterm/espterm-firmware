//
// Created by MightyPork on 2017/08/20.
//
// Handle CSI sequences
// CSI <symbol?> Pm <symbol?> <char>
// (CSI = ESC [)
//
// Example of those are cursor manipulation sequences and SGR.
//
// For details, see:
// http://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h2-Functions-using-CSI-_-ordered-by-the-final-character_s_
//
// Note:
// not all sequences listed in the xterm manual are implemented, notably sequences with the trailing symbol,
// graphic mode sequences, mouse reporting and complex multi-argument sequences that operate on regions.
//
// The screen size can be set using the xterm sequence: CSI Py ; Px t
//

#include <esp8266.h>
#include "apars_csi.h"
#include "screen.h"
#include "apars_logging.h"
#include "ansi_parser.h"
#include "ascii.h"
#include "ansi_parser_callbacks.h"
#include "uart_driver.h"
#include "sgr.h"
#include "version.h"
#include "syscfg.h"

// TODO simplify file - split to subroutines

static void warn_bad_csi()
{
	ansi_noimpl_r("Unknown CSI");
	apars_show_context();
}

typedef struct {
	char lead;
	const int *n;
	int count;
	char aug; // augmenting
	char key;
} CSI_Data;

static void do_csi_privattr(CSI_Data *opts);
static void do_csi_sgr(CSI_Data *opts);
static void do_csi_decreqtparm(CSI_Data *opts);

/**
 * Handle fully received CSI ANSI sequence
 * @param leadchar - private range leading character, 0 if none
 * @param params - array of CSI_N_MAX ints holding the numeric arguments
 * @param keychar - the char terminating the sequence
 */
void ICACHE_FLASH_ATTR
apars_handle_csi(char leadchar, const int *params, int count, char keychar)
{
	CSI_Data opts = {leadchar, params, count, NUL, keychar};
	char buf[32];
	bool yn = false; // for ? l h

	int n1 = params[0];
	int n2 = params[1];
	int n3 = params[2];

	// defaults - FIXME this may inadvertently affect some variants that should be left unchanged
	switch (keychar) {
		case 'A': // move
		case 'a':
		case 'e':
		case 'B':
		case 'C':
		case 'D':
		case 'b':
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
		case 'A': // Up
			screen_cursor_move(-n1, 0, false);
			break;

		case 'e':
		case 'B': // Down
			screen_cursor_move(n1, 0, false);
			break;

		case 'a': // some archaic form of "Go Right"
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

		case 'b':
			// TODO repeat preceding graphic character n1 times
			ansi_noimpl("Repeat char");
			return;

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
		case 'S':
			if (leadchar == NUL && count <= 1) {
				screen_scroll_up(n1);
			}
			else {
				// other:
				// CSI ? Pi; Pa; Pv S (sixel)
				warn_bad_csi();
			}
			break;

		case 'T':
			if (leadchar == NUL && count <= 1) {
				// CSI Ps T
				screen_scroll_down(n1);
			}
			else {
				// other:
				// CSI Ps ; Ps ; Ps ; Ps ; Ps T
				// CSI > Ps; Ps T
				warn_bad_csi();
			}
			break;

		case 't': // xterm window commands
			if (leadchar == NUL && count <= 2) {
				// CSI Ps ; Ps ; Ps t
				switch (n1) {
					case 8: // set size
						screen_resize(n2, n3);
						break;

					case 18: // report size
						printf(buf, "\033[8;%d;%dt", termconf_scratch.height, termconf_scratch.width);
						apars_respond(buf);
						break;

					case 21: // Report title
						apars_respond("\033]L");
						apars_respond(termconf_scratch.title);
						apars_respond("\033\\");
						break;

					case 24: // Set Height only
						screen_resize(n2, termconf_scratch.width);
						break;

					default:
						ansi_noimpl("CSI %d t", n1);
						break;
				}
			}
			else {
				// other:
				// CSI > Ps; Ps t
				// CSI Ps SP t,
				warn_bad_csi();
			}
			break;

			// CUP,HVP - set position
		case 'H':
		case 'f':
			screen_cursor_set(n1-1, n2-1);
			break; // 1-based

		case 'J': // Erase screen
			if (leadchar == '?') {
				// TODO selective erase
				ansi_noimpl("Selective erase");
			}

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
			if (leadchar == '?') {
				// TODO selective erase
				ansi_noimpl("Selective erase");
			}

			if (n1 == 0) {
				screen_clear_line(CLEAR_FROM_CURSOR);
			} else if (n1 == 1) {
				screen_clear_line(CLEAR_TO_CURSOR);
			} else {
				screen_clear_line(CLEAR_ALL);
			}
			break;

			// SCP, RCP - save/restore position
		case 's':
			if (leadchar == NUL && count == 0) {
				screen_cursor_save(0);
			}
			else if (leadchar == '?') {
				// Save private attributes (CSI ? Pm h/l)
				ansi_noimpl("Save private attrs");
			}
			else {
				// other:
				// CSI ? Pm s
				// CSI Pl; Pr s
				warn_bad_csi();
			}
			break;

		case 'r':
			if (leadchar == NUL && (count == 2 || count == 0)) {
				screen_set_scrolling_region(n1, n2);
			}
			else if (leadchar == '?') {
				// Restore private attributes (CSI ? Pm h/l)
				ansi_noimpl("Restore private attrs");
			}
			else {
				// other:
				// CSI ? Pm r
				// CSI Pt; Pl; Pb; Pr; Ps$ r
				warn_bad_csi();
			}
			break;

		case 'u':
			if (leadchar == NUL && count == 0) {
				screen_cursor_restore(0);
			}
			else {
				warn_bad_csi();
			}
			break;

		case 'h': // DEC feature enable
		case 'l': // DEC feature disable
			do_csi_privattr(&opts);
			break;

		case 'm': // SGR - set graphics rendition
			do_csi_sgr(&opts);
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

		case 'g': // Clear tabs
			if (n1 == 3) {
				screen_clear_all_tabs();
			}
			else if (n1 == 0) {
				screen_clear_tab();
			}
			break;

		case 'Z': // Tab backward
			screen_tab_reverse(n1);
			break;

		case 'I': // Tab forward
			screen_tab_forward(n1);
			break;

		case 'p':
			if (leadchar == '!') { // RIS
				/* On real VT there are differences between soft and hard reset, we treat both equally */
				screen_reset();
			}
			else {
				warn_bad_csi();
			}
			break;

		case 'n': // Queries
			if (leadchar == '>') {
				// some xterm garbage - discard
				// CSI > Ps n
				ansi_noimpl("CSI > %d n", n1);
				break;
			}

			if (leadchar == NUL) {
				if (n1 == 6) {
					// Query cursor position
					int x, y;
					screen_cursor_get(&y, &x);
					sprintf(buf, "\033[%d;%dR", y + 1, x + 1);
					apars_respond(buf);
				}
				else if (n1 == 5) {
					// Query device status - reply "Device is OK"
					apars_respond("\033[0n");
				}
				else {
					warn_bad_csi();
				}
			}
			else {
				warn_bad_csi();
			}
			break;

		case 'c': // CSI-c - report capabilities
			if (leadchar == NUL) {
				apars_respond("\033[?64;9c"); // pretend we're vt400 with national character sets
			}
			else if (leadchar == '>') {
				// 41 - we're "VT400", 0 - ROM cartridge number
				sprintf(buf, "\033[>41;%d;0c", FIRMWARE_VERSION_NUM);
				apars_respond(buf);
			}
			else {
				warn_bad_csi();
			}
			break;

		case 'x': // DECREQTPARM -> DECREPTPARM
			do_csi_decreqtparm(&opts);
			break;

		default:
			warn_bad_csi();
	}
}

/**
 * CSI [?] Pm {h|l}
 * @param opts
 */
static void ICACHE_FLASH_ATTR do_csi_privattr(CSI_Data *opts)
{
	bool yn = (opts->key == 'h');

	if (opts->lead == '?') {
		// --- DEC private attributes ---
		for (int i = 0; i < opts->count; i++) {
			int n = opts->n[i];
			if (n == 1) {
				screen_set_cursors_alt_mode(yn);
			}
			else if (n == 2) {
				// should reset all Gx to USASCII and reset to VT100 (which we use always)
				screen_set_charset(0, 'B');
				screen_set_charset(1, 'B');
			}
			else if (n == 3) {
				// 132 column mode - not implemented due to RAM demands
				ansi_noimpl("80->132");
			}
			else if (n == 4) {
				// Smooth scroll - not implemented
			}
			else if (n == 5) {
				screen_set_reverse_video(yn);
			}
			else if (n == 6) {
				screen_set_origin_mode(yn);
			}
			else if (n == 7) {
				screen_wrap_enable(yn);
			}
			else if (n == 8) {
				// Key auto-repeat
				// We don't implement this currently, but it could be added
				// - discard repeated keypress events between keydown and keyup.
				ansi_noimpl("Auto-repeat toggle");
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
				ansi_noimpl("Mouse tracking");
			}
			else if (n == 12) {
				// TODO Cursor blink on/off
				ansi_noimpl("Cursor blink toggle");
			}
			else if (n == 40) {
				// allow/disallow 80->132 mode
				// not implemented because of RAM demands
				ansi_noimpl("80->132 enable");
			}
			else if (n == 45) {
				// reverse wrap-around
				ansi_noimpl("Reverse Wraparound");
			}
			else if (n == 69) {
				// horizontal margins
				ansi_noimpl("Left/right margin");
			}
			else if (n == 47 || n == 1047) {
				// Switch to/from alternate screen
				//  - not implemented fully due to RAM demands
				screen_swap_state(yn);
			}
			else if (n == 1048) {
				// same as DECSC - save/restore cursor with attributes
				if (yn) {
					screen_cursor_save(true);
				}
				else {
					screen_cursor_restore(true);
				}
			}
			else if (n == 1049) {
				// save/restore cursor and screen and clear it
				if (yn) {
					screen_cursor_save(true);
					screen_swap_state(true); // this should save the screen - can't because of RAM size
					screen_clear(CLEAR_ALL);
				}
				else {
					screen_clear(CLEAR_ALL);
					screen_swap_state(false); // this should restore the screen - can't because of RAM size
					screen_cursor_restore(true);
				}
			}
			else if (n >= 1050 && n <= 1053) {
				// TODO Different kinds of function key emulation ?
				// (In practice this seems hardly ever used)

				// Ps = 1 0 5 0  -> Set terminfo/termcap function-key mode.
				// Ps = 1 0 5 1  -> Set Sun function-key mode.
				// Ps = 1 0 5 2  -> Set HP function-key mode.
				// Ps = 1 0 5 3  -> Set SCO function-key mode.
				ansi_noimpl("FN key emul type");
			}
			else if (n == 2004) {
				// Bracketed paste mode
				// Discard, we don't implement this
			}
			else if (n == 25) {
				screen_set_cursor_visible(yn);
			}
			else {
				ansi_noimpl("CSI ? %d %c", n, opts->key);
			}
		}
	}
	else {
		// --- DEC standard attributes ---
		for (int i = 0; i < opts->count; i++) {
			int n = opts->n[i];

			if (n == 4) {
				screen_set_insert_mode(yn);
			}
			else if (n == 20) {
				screen_set_newline_mode(yn);
			}
			else {
				ansi_noimpl("CSI %d %c", n, opts->key);
			}
		}
	}
}

/**
 * CSI [ Pm m
 * @param opts
 */
static void ICACHE_FLASH_ATTR do_csi_sgr(CSI_Data *opts)
{
	int count = opts->count;

	if (count == 0) {
		count = 1; // this makes it work as 0 (reset)
	}

	if (opts->lead != NUL) {
		// some xterm garbage - discard
		// CSI > Ps; Ps m
		return;
	}

	// iterate arguments
	for (int i = 0; i < count; i++) {
		int n = opts->n[i];

		if (n == SGR_RESET) screen_reset_sgr();
			// -- set color --
		else if (n >= SGR_FG_START && n <= SGR_FG_END) screen_set_fg((Color) (n - SGR_FG_START)); // ANSI normal fg
		else if (n >= SGR_BG_START && n <= SGR_BG_END) screen_set_bg((Color) (n - SGR_BG_START)); // ANSI normal bg
		else if (n == SGR_FG_DEFAULT) screen_set_fg(termconf_scratch.default_fg); // default fg
		else if (n == SGR_BG_DEFAULT) screen_set_bg(termconf_scratch.default_bg); // default bg
			// -- set attr --
		else if (n == SGR_BOLD) screen_set_sgr(ATTR_BOLD, 1);
		else if (n == SGR_FAINT) screen_set_sgr(ATTR_FAINT, 1);
		else if (n == SGR_ITALIC) screen_set_sgr(ATTR_ITALIC, 1);
		else if (n == SGR_UNDERLINE) screen_set_sgr(ATTR_UNDERLINE, 1);
		else if (n == SGR_BLINK || n == SGR_BLINK_FAST) screen_set_sgr(ATTR_BLINK, 1); // 6 - rapid blink, not supported
		else if (n == SGR_STRIKE) screen_set_sgr(ATTR_STRIKE, 1);
		else if (n == SGR_FRAKTUR) screen_set_sgr(ATTR_FRAKTUR, 1);
		else if (n == SGR_INVERSE) screen_set_sgr_inverse(1);
			// -- clear attr --
		else if (n == SGR_OFF(SGR_BOLD)) screen_set_sgr(ATTR_BOLD, 0); // can also mean "Double Underline"
		else if (n == SGR_OFF(SGR_FAINT)) screen_set_sgr(ATTR_FAINT | ATTR_BOLD, 0); // "normal"
		else if (n == SGR_OFF(SGR_ITALIC)) screen_set_sgr(ATTR_ITALIC | ATTR_FRAKTUR, 0); // there is no dedicated OFF code for Fraktur
		else if (n == SGR_OFF(SGR_UNDERLINE)) screen_set_sgr(ATTR_UNDERLINE, 0);
		else if (n == SGR_OFF(SGR_BLINK)) screen_set_sgr(ATTR_BLINK, 0);
		else if (n == SGR_OFF(SGR_STRIKE)) screen_set_sgr(ATTR_STRIKE, 0);
		else if (n == SGR_OFF(SGR_INVERSE)) screen_set_sgr_inverse(0);
			// -- AIX bright colors --
		else if (n >= SGR_FG_BRT_START && n <= SGR_FG_BRT_END) screen_set_fg((Color) ((n - SGR_FG_BRT_START) + 8)); // AIX bright fg
		else if (n >= SGR_BG_BRT_START && n <= SGR_BG_BRT_END) screen_set_bg((Color) ((n - SGR_BG_BRT_START) + 8)); // AIX bright bg
		else {
			ansi_noimpl("SGR %d", n);
		}
	}
}


// data tables for the DECREPTPARM command response

struct DECREPTPARM_parity { int parity; const char * msg; };
static const struct DECREPTPARM_parity DECREPTPARM_parity_arr[] = {
	{PARITY_NONE, "1"},
	{PARITY_ODD, "4"},
	{PARITY_EVEN, "5"},
	{-1, 0}
};

struct DECREPTPARM_baud { int baud; const char * msg; };
static const struct DECREPTPARM_baud DECREPTPARM_baud_arr[] = {
	{BIT_RATE_300, "48"},
	{BIT_RATE_600, "56"},
	{BIT_RATE_1200, "64"},
	{BIT_RATE_2400, "88"},
	{BIT_RATE_4800, "96"},
	{BIT_RATE_9600  , "104"},
	{BIT_RATE_19200 , "112"},
	{BIT_RATE_38400 , "120"},
	{BIT_RATE_57600 , "128"}, // this is the last in the spec, follow +8
	{BIT_RATE_74880 , "136"},
	{BIT_RATE_115200, "144"},
	{BIT_RATE_230400, "152"},
	{BIT_RATE_460800, "160"},
	{BIT_RATE_921600, "168"},
	{BIT_RATE_1843200, "176"},
	{BIT_RATE_3686400, "184"},
	{-1, 0}
};

/**
 * CSI [ Ps x
 * @param opts
 */
static void ICACHE_FLASH_ATTR do_csi_decreqtparm(CSI_Data *opts)
{
	const int n1 = opts->n[0];

	// reference http://vt100.net/docs/vt100-ug/chapter3.html - search DECREPTPARM
	if (n1 <= 1) {
		apars_respond("\033["); // this is a response on request (2 would be gratuitous)

		apars_respond(n1 == 0 ? "2;" : "3;");

		// Parity
		for(const struct DECREPTPARM_parity *p = DECREPTPARM_parity_arr; p->parity != -1; p++) {
			if (p->parity == sysconf->uart_parity) {
				apars_respond(p->msg);
				break;
			}
		}

		// bits per character (uart byte) - 1 = 8, 2 = 7
		apars_respond(";1;");

		// Baud rate
		for(const struct DECREPTPARM_baud *p = DECREPTPARM_baud_arr; p->baud != -1; p++) {
			if (p->baud == sysconf->uart_baudrate) {
				apars_respond(p->msg);
				apars_respond(";");
				apars_respond(p->msg);
				break;
			}
		}

		// multiplier 1, flags 0
		apars_respond(";1;0x"); // ROM cartridge number ??
	}
}
