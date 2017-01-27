#include <esp8266.h>
#include "screen.h"
#include "ansi_parser.h"

// Max nr of CSI parameters
#define CSI_N_MAX 3

/**
 * \brief Handle fully received CSI ANSI sequence
 * \param leadchar - private range leading character, 0 if none
 * \param params - array of CSI_N_MAX ints holding the numeric arguments
 * \param keychar - the char terminating the sequence
 */
void ICACHE_FLASH_ATTR
handle_CSI(char leadchar, int *params, char keychar)
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
		CSI 6n	DSR – Device Status Report  NOT IMPL
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
		case 'A': screen_cursor_move(0, -n1); break;
		case 'B': screen_cursor_move(0, n1);  break;
		case 'C': screen_cursor_move(n1, 0);  break;
		case 'D': screen_cursor_move(-n1, 0); break;

		case 'E': // CNL
			screen_cursor_move(0, n1);
			screen_cursor_set_x(0);
			break;

		case 'F': // CPL
			screen_cursor_move(0, -n1);
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
			screen_cursor_set(n2-1, n1-1); break; // 1-based

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
				screen_cursor_set_x(0);
			}
			break;

		// SCP, RCP
		case 's': screen_cursor_save(); break;
		case 'u': screen_cursor_restore(); break;

		// DECTCEM cursor show hide
		case 'l':
			if (leadchar == '?' && n1 == 25) {
				screen_cursor_enable(1);
			}
			break;

		case 'h':
			if (leadchar == '?' && n1 == 25) {
				screen_cursor_enable(0);
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

/**
 * \brief Handle a request to reset the display device
 */
void ICACHE_FLASH_ATTR
handle_RESET_cmd(void)
{
	screen_reset();
}

/**
 * \brief Handle a received plain character
 * \param c - the character
 */
void ICACHE_FLASH_ATTR
handle_plainchar(char c)
{
	screen_putchar(c);
}

void ICACHE_FLASH_ATTR
handle_OSC_FactoryReset(void)
{
	info("OSC: Factory reset");

	warn("NOT IMPLEMENTED");
	// TODO
}

void ICACHE_FLASH_ATTR
handle_OSC_SetScreenSize(int rows, int cols)
{
	info("OSC: Set screen size to %d x %d", rows, cols);

	screen_resize(rows, cols);
}


/* Ragel constants block */
%%{
	machine ansi;
	write data;
}%%

/**
 * \brief Linear ANSI chars stream parser
 *
 * Parses a stream of bytes using a Ragel parser. The defined
 * grammar does not use 'unget', so the entire buffer is
 * always processed in a linear manner.
 *
 * \attention -> but always check the Ragel output for 'p--'
 *            or 'p -=', that means trouble.
 *
 * \param newdata - array of new chars to process
 * \param len - length of the newdata buffer
 */
void ICACHE_FLASH_ATTR
ansi_parser(const char *newdata, size_t len)
{
	static int cs = -1;

	// The CSI code is built here
	static char csi_leading;      //!< Leading char, 0 if none
	static int  csi_ni;           //!< Number of the active digit
	static int  csi_n[CSI_N_MAX]; //!< Param digits
	static char csi_char;         //!< CSI action char (end)

	if (len == 0) len = strlen(newdata);
	
	// Load new data to Ragel vars
	const char *p = newdata;
	const char *eof = NULL;
	const char *pe = newdata + len;

	// Init Ragel on the first run
	if (cs == -1) {
		%% write init;
	}

	// The parser
	%%{
		ESC = 27;
		NOESC = (any - ESC);
		TOK_ST = ESC '\\'; # String terminator - used for OSC commands

		# --- Regular characters to be printed ---

		action plain_char {
			handle_plainchar(fc);
		}

		# --- CSI CSI commands (Select Graphic Rendition) ---
		# Text color & style

		action CSI_start {
			/* Reset the CSI builder */
			csi_leading = csi_char = 0;
			csi_ni = 0;

			/* Zero out digits */
			for(int i = 0; i < CSI_N_MAX; i++) {
				csi_n[i] = 0;
			}

			fgoto CSI_body;
		}

		action CSI_leading {
			csi_leading = fc;
		}

		action CSI_digit {
			/* x10 + digit */
			if (csi_ni < CSI_N_MAX) {
				csi_n[csi_ni] = csi_n[csi_ni]*10 + (fc - '0');
			}
		}

		action CSI_semi {
			csi_ni++;
		}

		action CSI_end {
			csi_char = fc;

			handle_CSI(csi_leading, csi_n, csi_char);

			fgoto main;
		}

		action errBadSeq {
			warn("Invalid escape sequence discarded.");
			fgoto main;
		}

		action back2main {
			fgoto main;
		}

		CSI_body := ((32..47|60..64) @CSI_leading)?
			((digit @CSI_digit)* ';' @CSI_semi)*
			(digit @CSI_digit)* alpha @CSI_end $!errBadSeq;


		# --- OSC commands (Operating System Commands) ---
		# Module parametrisation

		action OSC_start {
			csi_ni = 0;

			// we reuse the CSI numeric buffer
			for(int i = 0; i < CSI_N_MAX; i++) {
				csi_n[i] = 0;
			}

			fgoto OSC_body;
		}

		action OSC_fr {
			handle_OSC_FactoryReset();
			fgoto main;
		}

		action OSC_resize {
			handle_OSC_SetScreenSize(csi_n[0], csi_n[1]);

			fgoto main;
		}
		
		OSC_body := (
			("FR" ('\a' | ESC '\\') @OSC_fr) |
			('W' (digit @CSI_digit)+ ';' @CSI_semi (digit @CSI_digit)+ ('\a' | ESC '\\') @OSC_resize)
		) $!errBadSeq;

		action RESET_cmd {
			// Reset screen
			handle_RESET_cmd();
			fgoto main;
		}

		# --- Main parser loop ---

		main :=
			(
				(NOESC @plain_char)* ESC (
					'[' @CSI_start |
					']' @OSC_start |
					'c' @RESET_cmd
				)
			)+ $!errBadSeq;

		write exec;
	}%%
}
