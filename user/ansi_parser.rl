#include <esp8266.h>
#include "ansi_parser.h"
#include "screen.h"
#include "ascii.h"
#include "uart_driver.h"

/* Ragel constants block */
%%{
	machine ansi;
	write data;
}%%

static volatile int cs = -1;
static volatile bool inside_osc = false;

volatile u32 ansi_parser_char_cnt = 0;

void ICACHE_FLASH_ATTR
ansi_parser_reset(void) {
	if (cs != ansi_start) {
		cs = ansi_start;
		inside_osc = false;
		apars_reset_utf8buffer();
		ansi_warn("Parser timeout, state reset");
	}
}

#define HISTORY_LEN 10

#if DEBUG_ANSI
static char history[HISTORY_LEN + 1];
#endif

void ICACHE_FLASH_ATTR
apars_handle_badseq(void)
{
#if DEBUG_ANSI
	char buf1[HISTORY_LEN*3+2];
	char buf2[HISTORY_LEN*3+2];
	char *b1 = buf1;
	char *b2 = buf2;
	char c;

	for(int i=0;i<HISTORY_LEN;i++) {
		c = history[i];
		b1 += sprintf(b1, "%2X ", c);
		if (c < 32 || c > 127) c = '.';
		b2 += sprintf(b2, "%c  ", c);
	}

	ansi_dbg("Context: %s", buf2);
	ansi_dbg("         %s", buf1);
#endif
}

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
ansi_parser(char newchar)
{
	// The CSI code is built here
	static char csi_leading;      //!< Leading char, 0 if none
	static int  csi_ni;           //!< Number of the active digit
	static int  csi_cnt;          //!< Digit count
	static int  csi_n[CSI_N_MAX]; //!< Param digits
	static char csi_char;         //!< CSI action char (end)
	static char osc_buffer[OSC_CHAR_MAX];
	static int  osc_bi; // buffer char index

	// This is used to detect timeout delay (time since last rx char)
	ansi_parser_char_cnt++;

	// Init Ragel on the first run
	if (cs == -1) {
		%% write init;

		#if DEBUG_ANSI
			memset(history, 0, sizeof(history));
		#endif
	}

	#if DEBUG_ANSI
		for(int i=1; i<HISTORY_LEN; i++) {
			history[i-1] = history[i];
		}
		history[HISTORY_LEN-1] = newchar;
	#endif

	// Handle simple characters immediately (bypass parser)
	if (newchar < ' ') {
		switch (newchar) {
			case ESC:
				if (!inside_osc) {
					// Reset state
					cs = ansi_start;
					// now the ESC will be processed by the parser
				}
				break; // proceed to parser

				// Literally passed
			case FF:
			case VT:
				newchar = LF; // translate to LF, like VT100 / xterm do
			case CR:
			case LF:
			case TAB:
			case BS:
				apars_handle_plainchar(newchar);
				return;

				// Select G0 or G1
			case SI:
				screen_set_charset_n(1);
				return;
			case SO:
				screen_set_charset_n(0);
				return;

			case BEL:
				// bel is also used to terminate OSC
				if (!inside_osc) {
					apars_handle_bel();
					return;
				}
				break;

			case ENQ: // respond with space (like xterm)
				UART_WriteChar(UART0, SP, UART_TIMEOUT_US);
				return;

				// Cancel the active sequence
			case CAN:
			case SUB:
				cs = ansi_start;
				return;

			default:
				// Discard all others
				return;
		}
	} else {
		// >= ' '

		// bypass the parser for simple characters (speed-up)
		if (cs == ansi_start) {
			apars_handle_plainchar(newchar);
			return;
		}
	}
	
	// Load new data to Ragel vars
	const char *p = &newchar;
	const char *eof = NULL;
	const char *pe = &newchar + 1;

	// The parser
	%%{
#/*
		ESC = 27;
		NOESC = (any - ESC);
		TOK_ST = ESC '\\'; # String terminator - used for OSC commands
		OSC_END = ('\a' | TOK_ST);

		# --- Regular characters to be printed ---

		action plain_char {
			if (fc != 0) {
				apars_handle_plainchar(fc);
			}
		}

		# --- CSI CSI commands (Select Graphic Rendition) ---
		# Text color & style

		action CSI_start {
			// Reset the CSI builder
			csi_leading = csi_char = 0;
			csi_ni = 0;
			csi_cnt = 0;

			// Zero out digits
			for(int i = 0; i < CSI_N_MAX; i++) {
				csi_n[i] = 0;
			}

			fgoto CSI_body;
		}

		action CSI_leading {
			csi_leading = fc;
		}

		action CSI_digit {
			if (csi_cnt == 0) csi_cnt = 1;
			// x10 + digit
			if (csi_ni < CSI_N_MAX) {
				csi_n[csi_ni] = csi_n[csi_ni]*10 + (fc - '0');
			}
		}

		action CSI_semi {
			if (csi_cnt == 0) csi_cnt = 1; // handle case when first arg is empty
			csi_cnt++;
			csi_ni++;
		}

		action CSI_end {
			csi_char = fc;
			apars_handle_CSI(csi_leading, csi_n, csi_cnt, csi_char);
			fgoto main;
		}

		action errBadSeq {
			ansi_warn("Parser error.");
			apars_handle_badseq();
			fgoto main;
		}

		action back2main {
			fgoto main;
		}

		CSI_body := ((32..47|60..64) @CSI_leading)?
			((digit @CSI_digit)* ';' @CSI_semi)*
			(digit @CSI_digit)* (alpha|'`'|'@') @CSI_end $!errBadSeq;


		# --- OSC commands (Operating System Commands) ---
		# Module parametrisation

		action OSC_start {
			csi_ni = 0;

			// we reuse the CSI numeric buffer
			for(int i = 0; i < CSI_N_MAX; i++) {
				csi_n[i] = 0;
			}

			osc_bi = 0;
			osc_buffer[0] = '\0';

			inside_osc = true;

			fgoto OSC_body;
		}

		# collecting title string; this can also be entered by ESC k
		action SetTitle_start {
			osc_bi = 0;
			osc_buffer[0] = '\0';
			inside_osc = true;
			fgoto TITLE_body;
		}

		action OSC_resize {
			apars_handle_OSC_SetScreenSize(csi_n[0], csi_n[1]);
			inside_osc = false;
			fgoto main;
		}

		action OSC_text_char {
			osc_buffer[osc_bi++] = fc;
		}

		action OSC_title {
			osc_buffer[osc_bi++] = '\0';
			apars_handle_OSC_SetTitle(osc_buffer);
			inside_osc = false;
			fgoto main;
		}

		action OSC_button {
			osc_buffer[osc_bi++] = '\0';
			apars_handle_OSC_SetButton(csi_n[0], osc_buffer);
			inside_osc = false;
			fgoto main;
		}

		# 0; is xterm title hack
		OSC_body := (
			("BTN" digit @CSI_digit '=' (NOESC @OSC_text_char)* OSC_END @OSC_button) |
			("TITLE=" @SetTitle_start) |
			("0;" @SetTitle_start) |
			('W' (digit @CSI_digit)+ ';' @CSI_semi (digit @CSI_digit)+ OSC_END @OSC_resize)
		) $!errBadSeq;

		TITLE_body := (NOESC @OSC_text_char)* OSC_END @OSC_title $!errBadSeq;

		action RESET_cmd {
			// Reset screen
			apars_handle_RESET_cmd();
			fgoto main;
		}

		action CSI_SaveCursorAttrs {
			apars_handle_saveCursorAttrs();
			fgoto main;
		}

		action CSI_RestoreCursorAttrs {
			apars_handle_restoreCursorAttrs();
			fgoto main;
		}

		action HASH_code {
			apars_handle_hashCode(fc);
			fgoto main;
		}

		action SHORT_code {
			apars_handle_shortCode(fc);
			fgoto main;
		}

		action SetXCtrls {
			apars_handle_setXCtrls(fc); // weird control settings like 7 bit / 8 bit mode
			fgoto main;
		}

		action CharsetCmd_start {
			// abuse the buffer for storing the leading char
			osc_buffer[0] = fc;
			fgoto charsetcmd_body;
		}

		action CharsetCmd_end {
			apars_handle_characterSet(osc_buffer[0], fc);
			fgoto main;
		}

		charsetcmd_body := (any @CharsetCmd_end) $!errBadSeq;

		# --- Main parser loop ---

		main :=
			(
				(NOESC @plain_char)* ESC (
					('[' @CSI_start) |
					(']' @OSC_start) |
					('#' digit @HASH_code) |
					('k' @SetTitle_start) |
					([a-jl-zA-Z0-9=<>] @SHORT_code) |
					([()*+-./] @CharsetCmd_start) |
					(' ' [FG] @SetXCtrls)
				)
			)+ $!errBadSeq;

		write exec;
#*/
	}%%
}

// 'ESC k blah OSC_end' is a shortcut for setting title (k is defined in GNU screen as Title Definition String)
