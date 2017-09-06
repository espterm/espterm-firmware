//
// Created by MightyPork on 2017/08/20.
//
// Handle CSI sequences
// CSI <symbol?> Pm <symbol?> <char>
//
// Example of those are cursor manipulation sequences and SGR.
//
// For details, see:
// http://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h2-Functions-using-CSI-_-ordered-by-the-final-character_s_
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

/** Struct passed to subroutines */
typedef struct {
	char lead;
	const int *n;
	int count;
	char inter;
	char key;
} CSI_Data;

// Disambiguations
static inline void switch_csi_Plain(CSI_Data *opts);
static inline void switch_csi_NoLeadInterBang(CSI_Data *opts);
static inline void switch_csi_LeadGreater(CSI_Data *opts);
static inline void switch_csi_LeadQuest(CSI_Data *opts);
static inline void switch_csi_LeadEquals(CSI_Data *opts);

// Subroutines
static inline void do_csi_sgr(CSI_Data *opts);
static inline void do_csi_decreqtparm(CSI_Data *opts);
static inline void do_csi_set_option(CSI_Data *opts);
static inline void do_csi_xterm_screen_cmd(CSI_Data *opts);
static inline void do_csi_set_private_option(CSI_Data *opts);

/**
 * Show warning and dump context for invalid CSI
 */
static void warn_bad_csi(void)
{
	ansi_noimpl_r("Unknown CSI");
	apars_show_context();
}

/**
 * Handle fully received CSI ANSI sequence
 *
 * @param leadchar - leading character
 * @param params - array of CSI_N_MAX ints holding the numeric arguments
 * @param count - actual amount of received numeric arguments
 * @param keychar - intermediate character
 * @param keychar - final character
 */
void ICACHE_FLASH_ATTR
apars_handle_csi(char leadchar, const int *params, int count, char interchar, char keychar)
{
	CSI_Data opts = {leadchar, params, count, interchar, keychar};

	switch(leadchar) {
		case '?':
			switch_csi_LeadQuest(&opts);
			break;

		case '>':
			switch_csi_LeadGreater(&opts);
			break;

		case '=':
			switch_csi_LeadEquals(&opts);
			break;

		case NUL:
			// No leading character, switch by intermediate character
			switch(interchar) {
				case NUL:
					switch_csi_Plain(&opts);
					break;

				case '!':
					switch_csi_NoLeadInterBang(&opts);
					break;

//				case '\'':
//					switch_csi_NoLeadInterApos(opts);
//					break;

//				case '*':
//					switch_csi_NoLeadInterStar(opts);
//					break;

//				case '+':
//					switch_csi_NoLeadInterPlus(opts);
//					break;

//				case '"':
//					switch_csi_NoLeadInterQuote(opts);
//					break;

//				case '|':
//					switch_csi_NoLeadInterDollar(opts);
//					break;

//				case ' ':
//					switch_csi_NoLeadInterSpace(opts);
//					break;

//				case ',':
//					switch_csi_NoLeadInterComma(opts);
//					break;

//				case ')':
//					switch_csi_NoLeadInterRparen(opts);
//					break;

//				case '&':
//					switch_csi_NoLeadInterAmpers(opts);
//					break;

//				case '-':
//					switch_csi_NoLeadInterDash(opts);
//					break;

				default:
					warn_bad_csi();
			}
			break;

		default:
			warn_bad_csi();
	}
}


/**
 * CSI none Pm none key
 * @param opts
 */
static inline void ICACHE_FLASH_ATTR
switch_csi_Plain(CSI_Data *opts)
{
	char resp_buf[20];
	int n1 = opts->n[0];
	int n2 = opts->n[1];

	// fix arguments (default values etc)
	switch (opts->key) {
		// Single argument, 1-based
		case 'A': // up
		case 'e': // down (old)
		case 'B': // down
		case 'a': // right (old)
		case 'C': // right
		case 'D': // left
		case 'E': // cursor next line
		case 'F': // cursor prev line
		case 'b': // repeat last char
		case 'G': // set X
		case '`': // set X (alias)
		case 'd': // set Y
		case 'X': // clear in line
		case 'S': // scroll up
		case 'T': // scroll down
		case 'L': // Insert lines
		case 'M': // Delete lines
		case '@': // Insert in line
		case 'P': // Delete in line
		case 'I': // Tab forward
		case 'Z': // Tab backward
			if (n1 == 0) n1 = 1;
			break;

			// Two arguments, 1-based
		case 'H': // Absolute positioning
		case 'f':
			if (n1 == 0) n1 = 1;
			if (n2 == 0) n2 = 1;
			break;

			// Erase modes 0,1,2
		case 'J': // Erase in screen
		case 'K': // Erase in line
			if (n1 > 2) n1 = 0;
			break;

			// No defaults
		case 't': // Xterm window commands & reports
		case 's': // Cursor save no attr
		case 'r': // Set scrolling region
		case 'u': // Cursor restore no attr
		case 'h': // Option ON
		case 'l': // Option OFF
		case 'm': // SGR
		case 'g': // Clear tabs
		case 'n': // Queries 1 - device status
		case 'c': // Queries 2 - primary DA
		case 'x': // Queries 3 - DECREQTPARM
		default:
			// leave as is
			break;
	}

	switch (opts->key) {
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
			screen_scroll_up(n1);
			break;

		case 'T':
			screen_scroll_down(n1);
			break;

		case 't': // xterm window commands
			do_csi_xterm_screen_cmd(opts);
			break;

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
		case 's':
			screen_cursor_save(0);
			break;

		case 'r':
			screen_set_scrolling_region(n1-1, n2-1);
			break;

		case 'u':
			screen_cursor_restore(0);
			break;

		case 'h': // DEC feature enable
		case 'l': // DEC feature disable
			// --- DEC standard attributes ---
			do_csi_set_option(opts);
			break;

		case 'm': // SGR - set graphics rendition
			do_csi_sgr(opts);
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

		case 'n': // Queries
			if (n1 == 6) {
				// Query cursor position
				int x, y;
				screen_cursor_get(&y, &x);
				sprintf(resp_buf, "\033[%d;%dR", y + 1, x + 1);
				apars_respond(resp_buf);
			}
			else if (n1 == 5) {
				// Query device status - reply "Device is OK"
				apars_respond("\033[0n");
			}
			else {
				warn_bad_csi();
			}
			break;

		case 'c': // CSI-c - report capabilities
			// Primary device attributes
			apars_respond("\033[?64;22;9c"); // pretend we're vt420 with national character sets and colors.
			break;

		case 'x': // DECREQTPARM -> DECREPTPARM
			do_csi_decreqtparm(opts);
			break;

		default:
			warn_bad_csi();
	}
}


/**
 * CSI none Pm ! key
 */
static inline void  ICACHE_FLASH_ATTR
switch_csi_NoLeadInterBang(CSI_Data *opts)
{
	switch(opts->key) {
		case 'p':
			// RIS - CSI ! p
			// On real VT there are differences between soft and hard reset, we treat both equally
			screen_reset();
			break;

		default:
			warn_bad_csi();
	}
}


/**
 * CSI > Pm inter key
 */
static inline void ICACHE_FLASH_ATTR
switch_csi_LeadGreater(CSI_Data *opts)
{
	char resp_buf[20];
	switch(opts->key) {
		case 'c': // CSI > c - secondary device attributes query
			// 41 - we're "VT400", FV wers, 0 - ROM cartridge number
			sprintf(resp_buf, "\033[>41;%d;0c", FIRMWARE_VERSION_NUM);
			apars_respond(resp_buf);
			break;

		default:
			warn_bad_csi();
	}
}


/**
 * CSI = Pm inter key
 */
static inline void ICACHE_FLASH_ATTR
switch_csi_LeadEquals(CSI_Data *opts)
{
	char resp_buf[20];
	u8 mac[6];
	switch(opts->key) {
		case 'c': // CSI = c - tertiary device attributes query
			// report our unique ID number
			wifi_get_macaddr(SOFTAP_IF, mac);
			sprintf(resp_buf, "\033P!|%02X%02X%02X\033\\", mac[3], mac[4], mac[5]);
			apars_respond(resp_buf);
			break;

		default:
			warn_bad_csi();
	}
}

/**
 * CSI ? Pm inter key
 */
static inline void  ICACHE_FLASH_ATTR
switch_csi_LeadQuest(CSI_Data *opts)
{
	switch(opts->key) {
		case 's':
			// Save private attributes
			ansi_noimpl("Save private attrs");
			apars_show_context(); // TODO priv attr s/r
			break;

		case 'r':
			// Restore private attributes
			ansi_noimpl("Restore private attrs");
			apars_show_context(); // TODO priv attr s/r
			break;

		case 'J': // Erase screen selectively
			// TODO selective erase
			ansi_noimpl("Selective screen erase");
			break;

		case 'K': // Erase line selectively
			// TODO selective erase
			ansi_noimpl("Selective line erase");
			break;

		case 'l':
		case 'h':
			do_csi_set_private_option(opts);
			break;

		default:
			warn_bad_csi();
	}
}


/**
 * CSI Pm m
 * @param opts
 */
static inline void ICACHE_FLASH_ATTR
do_csi_sgr(CSI_Data *opts)
{
	int count = opts->count;

	if (count == 0) {
		count = 1; // this makes it work as 0 (reset)
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


/**
 * CSI Pm h or l
 * @param opts
 */
static inline void ICACHE_FLASH_ATTR
do_csi_set_option(CSI_Data *opts)
{
	bool yn = (opts->key == 'h');

	for (int i = 0; i < opts->count; i++) {
		int n = opts->n[i];

		if (n == 4) {
			screen_set_insert_mode(yn);
		}
		else if (n == 12) {
			// SRM is inverted, according to vt510 manual
			termconf_scratch.loopback = !yn;
		}
		else if (n == 20) {
			screen_set_newline_mode(yn);
		}
		else {
			ansi_noimpl("OPTION %d", n);
		}
	}
}


/**
 * CSI ? Pm h or l
 * @param opts
 */
static inline void ICACHE_FLASH_ATTR
do_csi_set_private_option(CSI_Data *opts)
{
	bool yn = (opts->key == 'h');

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
			// DECCOLM 132 column mode - not implemented due to RAM demands
			ansi_noimpl("132col");

			// DECCOLM side effects as per
			// https://www.chiark.greenend.org.uk/~sgtatham/putty/wishlist/deccolm-cls.html
			screen_clear(CLEAR_ALL);
			screen_set_scrolling_region(0, 0);
			screen_cursor_set(0, 0);
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
		else if (n == 9 || (n >= 1000 && n <= 1006) || n == 1015) {
			// 9 - X10 tracking
			// 1000 - X11 mouse - Send Mouse X & Y on button press and release.
			// 1001 - Hilite mouse tracking - not impl
			// 1002 - Cell Motion Mouse Tracking
			// 1003 - All Motion Mouse Tracking
			// 1004 - Send FocusIn/FocusOut events
			// 1005 - Enable UTF-8 Mouse Mode - we implement this as an alias to X10 mode
			// 1006 - SGR mouse mode
			if (n == 9) mouse_tracking.mode = yn ? MTM_X10 : MTM_NONE;
			else if (n == 1000) mouse_tracking.mode = yn ? MTM_NORMAL : MTM_NONE;
			else if (n == 1002) mouse_tracking.mode = yn ? MTM_BUTTON_MOTION : MTM_NONE;
			else if (n == 1003) mouse_tracking.mode = yn ? MTM_ANY_MOTION : MTM_NONE;
			else if (n == 1004) mouse_tracking.focus_tracking = yn;
			else if (n == 1005) mouse_tracking.encoding = yn ? MTE_UTF8 : MTE_SIMPLE;
			else if (n == 1006) mouse_tracking.encoding = yn ? MTE_SGR : MTE_SIMPLE;
			else if (n == 1015) mouse_tracking.encoding = yn ? MTE_URXVT : MTE_SIMPLE;

			dbg("Mouse mode=%d, enc=%d, foctr=%d",
				mouse_tracking.mode,
				mouse_tracking.encoding,
				mouse_tracking.focus_tracking);
		}
		else if (n == 12) {
			// TODO Cursor blink on/off
			ansi_noimpl("Cursor blink toggle");
		}
		else if (n == 40) {
			// allow/disallow 80->132 mode
			// not implemented because of RAM demands
			ansi_noimpl("132col enable");
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
		else if (n == 2004) {
			// Bracketed paste mode
			ansi_noimpl("Bracketed paste");
		}
		else if (n == 25) {
			screen_set_cursor_visible(yn);
		}
		else if (n == 800) { // ESPTerm: Toggle display of buttons
			termconf_scratch.show_buttons = yn;
			screen_notifyChange(CHANGE_CONTENT); // this info is included in the screen preamble
		}
		else if (n == 801) { // ESPTerm: Toggle display of config links
			termconf_scratch.show_config_links = yn;
			screen_notifyChange(CHANGE_CONTENT); // this info is included in the screen preamble
		}
		else {
			ansi_noimpl("?OPTION %d", n);
		}
	}
}


/**
 * CSI Ps ; Ps ; Ps t
 * @param opts
 */
static inline void ICACHE_FLASH_ATTR
do_csi_xterm_screen_cmd(CSI_Data *opts)
{
	char resp_buf[20];
	switch (opts->n[0]) {
		case 8: // set size
			screen_resize(opts->n[1], opts->n[2]);
			break;

		case 18: // Report the size of the text area in characters.
		case 19: // Report the size of the screen in characters.
			sprintf(resp_buf, "\033[8;%d;%dt", termconf_scratch.height, termconf_scratch.width);
			apars_respond(resp_buf);
			break;

		case 20: // Report icon
		case 21: // Report title
			apars_respond("\033]l");
			apars_respond(termconf_scratch.title);
			apars_respond("\033\\");
			break;

		case 22:
			ansi_noimpl("Push title");
			break;
		case 23:
			ansi_noimpl("Pop title");
			break;

		case 24: // Set Height only
			screen_resize(opts->n[1], termconf_scratch.width);
			break;

		default:
			ansi_noimpl("Xterm win report %d", opts->n[0]);
			break;
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
 * CSI Ps x
 * @param opts
 */
static inline void ICACHE_FLASH_ATTR
do_csi_decreqtparm(CSI_Data *opts)
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
