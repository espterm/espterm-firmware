#include <esp8266.h>
#include "ansi_parser.h"
#include "screen.h"

/* Ragel constants block */
%%{
	machine ansi;
	write data;
}%%

static volatile int cs = -1;

static ETSTimer resetTim;

static void ICACHE_FLASH_ATTR
resetParserCb(void *arg) {
	if (cs != ansi_start) {
		cs = ansi_start;
		warn("Parser timeout, state reset");
	}
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
ansi_parser(const char *newdata, size_t len)
{
	// The CSI code is built here
	static char csi_leading;      //!< Leading char, 0 if none
	static int  csi_ni;           //!< Number of the active digit
	static int  csi_n[CSI_N_MAX]; //!< Param digits
	static char csi_char;         //!< CSI action char (end)
	static char osc_buffer[OSC_CHAR_MAX];
	static int  osc_bi;

	if (len == 0) len = strlen(newdata);
	
	// Load new data to Ragel vars
	const char *p = newdata;
	const char *eof = NULL;
	const char *pe = newdata + len;

	// Init Ragel on the first run
	if (cs == -1) {
		%% write init;
	}

	// schedule state reset
	if (termconf->parser_tout_ms > 0) {
		os_timer_disarm(&resetTim);
		os_timer_setfn(&resetTim, resetParserCb, NULL);
		os_timer_arm(&resetTim, termconf->parser_tout_ms, 0);
	}

	// The parser
	%%{
#/*
		ESC = 27;
		NOESC = (any - ESC);
		TOK_ST = ESC '\\'; # String terminator - used for OSC commands
		OSC_END = ('\a' | ESC '\\');

		# --- Regular characters to be printed ---

		action plain_char {
			apars_handle_plainchar(fc);
		}

		# --- CSI CSI commands (Select Graphic Rendition) ---
		# Text color & style

		action CSI_start {
			// Reset the CSI builder
			csi_leading = csi_char = 0;
			csi_ni = 0;

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
			// x10 + digit
			if (csi_ni < CSI_N_MAX) {
				csi_n[csi_ni] = csi_n[csi_ni]*10 + (fc - '0');
			}
		}

		action CSI_semi {
			csi_ni++;
		}

		action CSI_end {
			csi_char = fc;

			apars_handle_CSI(csi_leading, csi_n, csi_char);

			fgoto main;
		}

		action errBadSeq {
			apars_handle_badseq();
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

			osc_bi = 0;
			osc_buffer[0] = '\0';

			fgoto OSC_body;
		}

		action OSC_resize {
			apars_handle_OSC_SetScreenSize(csi_n[0], csi_n[1]);
			fgoto main;
		}

		action OSC_text_char {
			osc_buffer[osc_bi++] = fc;
		}

		action OSC_title {
			osc_buffer[osc_bi++] = '\0';
			apars_handle_OSC_SetTitle(osc_buffer);
			fgoto main;
		}

		action OSC_button {
			osc_buffer[osc_bi++] = '\0';
			apars_handle_OSC_SetButton(csi_n[0], osc_buffer);
			fgoto main;
		}
		
		OSC_body := (
			("BTN" digit @CSI_digit '=' (NOESC @OSC_text_char)+ OSC_END @OSC_button) |
			("TITLE=" (NOESC @OSC_text_char)+ OSC_END @OSC_title) |
			('W' (digit @CSI_digit)+ ';' @CSI_semi (digit @CSI_digit)+ OSC_END @OSC_resize)
		) $!errBadSeq;

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

		# --- Main parser loop ---

		main :=
			(
				(NOESC @plain_char)* ESC (
					'[' @CSI_start |
					']' @OSC_start |
					'c' @RESET_cmd |
			        '7' @CSI_SaveCursorAttrs |
					'8' @CSI_RestoreCursorAttrs
				)
			)+ $!errBadSeq;

		write exec;
#*/
	}%%
}
