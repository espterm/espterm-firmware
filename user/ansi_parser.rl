#include <esp8266.h>
#include "ansi_parser.h"
#include "ansi_parser_callbacks.h"
#include "ascii.h"
#include "apars_logging.h"

/* Ragel constants block */
%%{
	machine ansi;
	write data;
}%%

// Max nr of CSI parameters
#define CSI_N_MAX 10
#define STR_CHAR_MAX 64

static volatile int cs = -1;
static volatile bool inside_string = false;

// public
volatile u32 ansi_parser_char_cnt = 0;

void ICACHE_FLASH_ATTR
ansi_parser_reset(void) {
	if (cs != ansi_start) {
		cs = ansi_start;
		inside_string = false;
		apars_reset_utf8buffer();
		ansi_warn("Parser timeout, state reset");
	}
}

#define HISTORY_LEN 10

#if DEBUG_ANSI
static char history[HISTORY_LEN + 1];
#endif

void ICACHE_FLASH_ATTR
apars_show_context(void)
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
	static char leadchar;
	static int  arg_ni;
	static int  arg_cnt;
	static int  arg[CSI_N_MAX];
	static char string_buffer[STR_CHAR_MAX];
	static int  str_ni;

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
				if (!inside_string) {
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
			case BS:
				apars_handle_plainchar(newchar);
				return;

			case TAB:
				apars_handle_tab();
				return;

				// Select G0 or G1
			case SI:
				apars_handle_chs_switch(1);
				return;

			case SO:
				apars_handle_chs_switch(0);
				return;

			case BEL:
				// bel is also used to terminate OSC
				if (!inside_string) {
					apars_handle_bel();
					return;
				}
				break;

			case ENQ:
				apars_handle_enq();
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
		NOESC = (any - ESC - 7);
		TOK_ST = ESC '\\'; # String terminator - used for OSC commands
		STR_END = (7 | TOK_ST);

		# --- Error handler ---

		action errBadSeq {
			ansi_warn("Parser error.");
			apars_show_context();
			inside_string = false; // no longer in string, for sure
			fgoto main;
		}

		# --- Regular characters to be printed ---

		action plain_char {
			if (fc != 0) {
				apars_handle_plainchar(fc);
			}
		}

		# --- CSI commands ---

		action CSI_start {
			// Reset the CSI builder
			leadchar = NUL;
			arg_ni = 0;
			arg_cnt = 0;

			// Zero out digits
			for(int i = 0; i < CSI_N_MAX; i++) {
				arg[i] = 0;
			}

			fgoto CSI_body;
		}

		action CSI_leading {
			leadchar = fc;
		}

		action CSI_digit {
			if (arg_cnt == 0) arg_cnt = 1;
			// x10 + digit
			if (arg_ni < CSI_N_MAX) {
				arg[arg_ni] = arg[arg_ni]*10 + (fc - '0');
			}
		}

		action CSI_semi {
			if (arg_cnt == 0) arg_cnt = 1; // handle case when first arg is empty
			arg_cnt++;
			arg_ni++;
		}

		action CSI_end {
			apars_handle_csi(leadchar, arg, arg_cnt, fc);
			fgoto main;
		}

		CSI_body := ((32..47|60..64) @CSI_leading)?
			((digit @CSI_digit)* ';' @CSI_semi)*
			(digit @CSI_digit)* (alpha|'`'|'@') @CSI_end $!errBadSeq;

		# --- String commands ---

		action StrCmd_start {
			leadchar = fc;
			str_ni = 0;
			string_buffer[0] = '\0';
			inside_string = true;
			fgoto STRCMD_body;
		}

		action StrCmd_char {
			string_buffer[str_ni++] = fc;
		}

		action StrCmd_end {
			inside_string = false;
			string_buffer[str_ni++] = '\0';
			apars_handle_string_cmd(leadchar, string_buffer);
			fgoto main;
		}

		# According to the spec, ESC should be allowed inside the string sequence.
		# We disallow ESC for simplicity, as it's hardly ever used.
		STRCMD_body := ((NOESC @StrCmd_char)* STR_END @StrCmd_end) $!errBadSeq;

		# --- Single character ESC ---

		action HASH_code {
			apars_handle_hash_cmd(fc);
			fgoto main;
		}

		action SHORT_code {
			apars_handle_short_cmd(fc);
			fgoto main;
		}

		action SPACE_cmd {
			apars_handle_space_cmd(fc);
			fgoto main;
		}

		# --- Charset selection ---

		action CharsetCmd_start {
			leadchar = fc;
			fgoto charsetcmd_body;
		}

		action CharsetCmd_end {
			apars_handle_chs_designate(leadchar, fc);
			fgoto main;
		}

		charsetcmd_body := (NOESC @CharsetCmd_end) $!errBadSeq;

		# --- Main parser loop ---

		main :=
			(
				(NOESC @plain_char)* ESC (
					('[' @CSI_start) |
					([_\]Pk\^X] @StrCmd_start) |
					('#' digit @HASH_code) |
					(([a-zA-Z0-9=<>~}|@\\] - [PXk]) @SHORT_code) |
					([()*+-./%] @CharsetCmd_start) |
					(' ' [FGLMN] @SPACE_cmd)
				)
			)+ $!errBadSeq;

		write exec;
#*/
	}%%
}
