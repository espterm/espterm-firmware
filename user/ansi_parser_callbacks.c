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

/**
 * Handle a received plain character
 */
void ICACHE_FLASH_ATTR
apars_handle_plainchar(char c)
{
	screen_putchar(c);
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

			// DECTCEM cursor show hide
		case 'l':
			if (leadchar == '?' && n1 == 25) {
				screen_cursor_enable(0);
			}
			break;

		case 'h':
			if (leadchar == '?' && n1 == 25) {
				screen_cursor_enable(1);
			}
			break;

		case 'm': // SGR
			// iterate arguments
			for (int i = 0; i < CSI_N_MAX; i++) {
				int n = params[i];

				if (i == 0 && n == 0) { // reset SGR
					screen_set_fg(7);
					screen_set_bg(0);
					break; // cannot combine reset with others
				}
				else if (n >= 30 && n <= 37) screen_set_fg(n-30); // ANSI normal fg
				else if (n >= 40 && n <= 47) screen_set_bg(n-40); // ANSI normal bg
				else if (n == 39) screen_set_fg(7); // default fg
				else if (n == 49) screen_set_bg(0); // default bg
				else if (n == 7) screen_inverse(1); // inverse
				else if (n == 27) screen_inverse(0); // positive
				else if (n == 1) screen_set_bright_fg(); // ANSI bold = bright fg
				else if (n >= 90 && n <= 97) screen_set_fg(n-90+8); // AIX bright fg
				else if (n >= 100 && n <= 107) screen_set_bg(n-100+8); // AIX bright bg
			}
			break;
	}
}

void ICACHE_FLASH_ATTR apars_handle_saveCursorAttrs(void)
{
	screen_cursor_save(1);
}

void ICACHE_FLASH_ATTR apars_handle_restoreCursorAttrs(void)
{
	screen_cursor_restore(1);
}

/**
 * \brief Handle a request to reset the display device
 */
void ICACHE_FLASH_ATTR
apars_handle_RESET_cmd(void)
{
	// XXX maybe user wanted to reset the module instead?
	screen_reset();
}

/**
 * Handle a factory reset request
 */
void ICACHE_FLASH_ATTR
apars_handle_OSC_FactoryReset(void)
{
	info("OSC: Factory reset");

	// Send acknowledgement message to UART0
	// User is performing this manually, so we can just print it as string
	UART_WriteString(UART0, "\r\nFACTORY RESET\r\n", UART_TIMEOUT_US);

	// Disconnect from AP if connected
	int opmode = wifi_get_opmode();
	if (opmode != SOFTAP_MODE) {
		wifi_station_disconnect();
	}

	// Both must be enabled so we can manipulate their settings
	wifi_set_opmode(STATIONAP_MODE);

	// --- AP config ---
	struct softap_config apconf;
	wifi_softap_get_config(&apconf);
	apconf.authmode=AUTH_OPEN; // Disable access protection
	apconf.channel=1; // Reset channel; user may have set bad channel in the UI

	// generate unique AP name
	u8 mac[6];
	wifi_get_macaddr(SOFTAP_IF, mac);
	sprintf((char*)apconf.ssid, "TERM-%02X%02X%02X", mac[3], mac[4], mac[5]);
	apconf.ssid_len = (u8)strlen((char*)apconf.ssid);

	// --- Station ---
	struct station_config staconf;
	wifi_station_get_config(&staconf);

	// clear info about SSID
	staconf.ssid[0]=0;
	staconf.bssid_set=0;
	staconf.password[0]=0;

	wifi_softap_set_config(&apconf);
	wifi_station_set_config(&staconf);

	UART_WriteString(UART0, "Factory Reset complete, device reset.\r\n\r\n", UART_TIMEOUT_US);

	// Reboot to clean STA+AP mode with Channel 1 & reset AP SSID.
	system_restart();
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
