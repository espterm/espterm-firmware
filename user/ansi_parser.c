
/* #line 1 "user/ansi_parser.rl" */
#include <esp8266.h>
#include "ansi_parser.h"
#include "screen.h"
#include "ascii.h"
#include "uart_driver.h"

/* Ragel constants block */

/* #line 12 "user/ansi_parser.c" */
static const char _ansi_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	3, 1, 4, 1, 5, 1, 6, 1, 
	7, 1, 8, 1, 9, 1, 10, 1, 
	11, 1, 12, 1, 13, 1, 14, 1, 
	15, 1, 16, 1, 17, 2, 2, 5, 
	2, 10, 11, 2, 10, 12
};

static const char _ansi_eof_actions[] = {
	0, 13, 13, 13, 13, 13, 13, 13, 
	13, 13, 13, 13, 13, 13, 13, 13, 
	13, 13, 13, 13, 13, 13, 13, 13, 
	13, 13, 13, 13, 13, 13, 0, 0, 
	0, 0, 0, 0, 0, 0, 0
};

static const int ansi_start = 1;
static const int ansi_first_final = 30;
static const int ansi_error = 0;

static const int ansi_en_CSI_body = 5;
static const int ansi_en_OSC_body = 7;
static const int ansi_en_TITLE_body = 27;
static const int ansi_en_charsetcmd_body = 29;
static const int ansi_en_main = 1;


/* #line 11 "user/ansi_parser.rl" */


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

#define HISTORY_LEN 16

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
		
/* #line 118 "user/ansi_parser.c" */
	{
	cs = ansi_start;
	}

/* #line 87 "user/ansi_parser.rl" */

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
				apars_handle_bel();
				return;

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
	
/* #line 202 "user/ansi_parser.c" */
	{
	const char *_acts;
	unsigned int _nacts;

	if ( p == pe )
		goto _test_eof;
	if ( cs == 0 )
		goto _out;
_resume:
	switch ( cs ) {
case 1:
	if ( (*p) == 27 )
		goto tr1;
	goto tr0;
case 2:
	switch( (*p) ) {
		case 32: goto tr3;
		case 35: goto tr4;
		case 91: goto tr7;
		case 93: goto tr8;
		case 107: goto tr9;
	}
	if ( (*p) < 60 ) {
		if ( (*p) > 47 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto tr6;
		} else if ( (*p) >= 40 )
			goto tr5;
	} else if ( (*p) > 62 ) {
		if ( (*p) > 90 ) {
			if ( 97 <= (*p) && (*p) <= 122 )
				goto tr6;
		} else if ( (*p) >= 65 )
			goto tr6;
	} else
		goto tr6;
	goto tr2;
case 0:
	goto _out;
case 3:
	if ( 70 <= (*p) && (*p) <= 71 )
		goto tr10;
	goto tr2;
case 30:
	if ( (*p) == 27 )
		goto tr1;
	goto tr0;
case 4:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr11;
	goto tr2;
case 5:
	switch( (*p) ) {
		case 59: goto tr14;
		case 64: goto tr15;
	}
	if ( (*p) < 60 ) {
		if ( (*p) > 47 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto tr13;
		} else if ( (*p) >= 32 )
			goto tr12;
	} else if ( (*p) > 63 ) {
		if ( (*p) > 90 ) {
			if ( 96 <= (*p) && (*p) <= 122 )
				goto tr16;
		} else if ( (*p) >= 65 )
			goto tr16;
	} else
		goto tr12;
	goto tr2;
case 6:
	if ( (*p) == 59 )
		goto tr14;
	if ( (*p) < 64 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr13;
	} else if ( (*p) > 90 ) {
		if ( 96 <= (*p) && (*p) <= 122 )
			goto tr16;
	} else
		goto tr16;
	goto tr2;
case 31:
	goto tr2;
case 32:
	if ( (*p) == 59 )
		goto tr14;
	if ( (*p) < 64 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr13;
	} else if ( (*p) > 90 ) {
		if ( 96 <= (*p) && (*p) <= 122 )
			goto tr16;
	} else
		goto tr16;
	goto tr2;
case 7:
	switch( (*p) ) {
		case 48: goto tr17;
		case 66: goto tr18;
		case 84: goto tr19;
		case 87: goto tr20;
	}
	goto tr2;
case 8:
	if ( (*p) == 59 )
		goto tr21;
	goto tr2;
case 9:
	switch( (*p) ) {
		case 7: goto tr23;
		case 27: goto tr24;
	}
	goto tr22;
case 33:
	switch( (*p) ) {
		case 7: goto tr23;
		case 27: goto tr24;
	}
	goto tr22;
case 10:
	if ( (*p) == 92 )
		goto tr25;
	goto tr2;
case 34:
	goto tr2;
case 11:
	if ( (*p) == 84 )
		goto tr26;
	goto tr2;
case 12:
	if ( (*p) == 78 )
		goto tr27;
	goto tr2;
case 13:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr28;
	goto tr2;
case 14:
	if ( (*p) == 61 )
		goto tr29;
	goto tr2;
case 15:
	switch( (*p) ) {
		case 7: goto tr31;
		case 27: goto tr32;
	}
	goto tr30;
case 35:
	switch( (*p) ) {
		case 7: goto tr31;
		case 27: goto tr32;
	}
	goto tr30;
case 16:
	if ( (*p) == 92 )
		goto tr33;
	goto tr2;
case 17:
	if ( (*p) == 73 )
		goto tr34;
	goto tr2;
case 18:
	if ( (*p) == 84 )
		goto tr35;
	goto tr2;
case 19:
	if ( (*p) == 76 )
		goto tr36;
	goto tr2;
case 20:
	if ( (*p) == 69 )
		goto tr37;
	goto tr2;
case 21:
	if ( (*p) == 61 )
		goto tr38;
	goto tr2;
case 22:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr39;
	goto tr2;
case 23:
	if ( (*p) == 59 )
		goto tr40;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr39;
	goto tr2;
case 24:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr41;
	goto tr2;
case 25:
	switch( (*p) ) {
		case 7: goto tr42;
		case 27: goto tr43;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr41;
	goto tr2;
case 26:
	if ( (*p) == 92 )
		goto tr42;
	goto tr2;
case 27:
	switch( (*p) ) {
		case 7: goto tr45;
		case 27: goto tr46;
	}
	goto tr44;
case 36:
	switch( (*p) ) {
		case 7: goto tr45;
		case 27: goto tr46;
	}
	goto tr44;
case 28:
	if ( (*p) == 92 )
		goto tr47;
	goto tr2;
case 37:
	goto tr2;
case 29:
	goto tr48;
case 38:
	goto tr2;
	}

	tr2: cs = 0; goto f0;
	tr0: cs = 1; goto f1;
	tr1: cs = 2; goto _again;
	tr3: cs = 3; goto _again;
	tr4: cs = 4; goto _again;
	tr12: cs = 6; goto f9;
	tr13: cs = 6; goto f10;
	tr14: cs = 6; goto f11;
	tr17: cs = 8; goto _again;
	tr21: cs = 9; goto _again;
	tr22: cs = 9; goto f14;
	tr24: cs = 10; goto _again;
	tr18: cs = 11; goto _again;
	tr26: cs = 12; goto _again;
	tr27: cs = 13; goto _again;
	tr28: cs = 14; goto f10;
	tr29: cs = 15; goto _again;
	tr30: cs = 15; goto f14;
	tr32: cs = 16; goto _again;
	tr19: cs = 17; goto _again;
	tr34: cs = 18; goto _again;
	tr35: cs = 19; goto _again;
	tr36: cs = 20; goto _again;
	tr37: cs = 21; goto _again;
	tr20: cs = 22; goto _again;
	tr39: cs = 23; goto f10;
	tr40: cs = 24; goto f11;
	tr41: cs = 25; goto f10;
	tr43: cs = 26; goto _again;
	tr44: cs = 27; goto f14;
	tr46: cs = 28; goto _again;
	tr5: cs = 30; goto f2;
	tr6: cs = 30; goto f3;
	tr7: cs = 30; goto f4;
	tr8: cs = 30; goto f5;
	tr9: cs = 30; goto f6;
	tr10: cs = 30; goto f7;
	tr11: cs = 30; goto f8;
	tr16: cs = 31; goto f13;
	tr15: cs = 32; goto f12;
	tr23: cs = 33; goto f15;
	tr38: cs = 34; goto f6;
	tr25: cs = 34; goto f16;
	tr33: cs = 34; goto f18;
	tr42: cs = 34; goto f19;
	tr31: cs = 35; goto f17;
	tr45: cs = 36; goto f15;
	tr47: cs = 37; goto f16;
	tr48: cs = 38; goto f20;

	f1: _acts = _ansi_actions + 1; goto execFuncs;
	f4: _acts = _ansi_actions + 3; goto execFuncs;
	f9: _acts = _ansi_actions + 5; goto execFuncs;
	f10: _acts = _ansi_actions + 7; goto execFuncs;
	f11: _acts = _ansi_actions + 9; goto execFuncs;
	f13: _acts = _ansi_actions + 11; goto execFuncs;
	f0: _acts = _ansi_actions + 13; goto execFuncs;
	f5: _acts = _ansi_actions + 15; goto execFuncs;
	f6: _acts = _ansi_actions + 17; goto execFuncs;
	f19: _acts = _ansi_actions + 19; goto execFuncs;
	f14: _acts = _ansi_actions + 21; goto execFuncs;
	f16: _acts = _ansi_actions + 23; goto execFuncs;
	f18: _acts = _ansi_actions + 25; goto execFuncs;
	f8: _acts = _ansi_actions + 27; goto execFuncs;
	f3: _acts = _ansi_actions + 29; goto execFuncs;
	f7: _acts = _ansi_actions + 31; goto execFuncs;
	f2: _acts = _ansi_actions + 33; goto execFuncs;
	f20: _acts = _ansi_actions + 35; goto execFuncs;
	f12: _acts = _ansi_actions + 37; goto execFuncs;
	f15: _acts = _ansi_actions + 40; goto execFuncs;
	f17: _acts = _ansi_actions + 43; goto execFuncs;

execFuncs:
	_nacts = *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 0:
/* #line 173 "user/ansi_parser.rl" */
	{
			if ((*p) != 0) {
				apars_handle_plainchar((*p));
			}
		}
	break;
	case 1:
/* #line 182 "user/ansi_parser.rl" */
	{
			// Reset the CSI builder
			csi_leading = csi_char = 0;
			csi_ni = 0;
			csi_cnt = 0;

			// Zero out digits
			for(int i = 0; i < CSI_N_MAX; i++) {
				csi_n[i] = 0;
			}

			{cs = 5;goto _again;}
		}
	break;
	case 2:
/* #line 196 "user/ansi_parser.rl" */
	{
			csi_leading = (*p);
		}
	break;
	case 3:
/* #line 200 "user/ansi_parser.rl" */
	{
			if (csi_cnt == 0) csi_cnt = 1;
			// x10 + digit
			if (csi_ni < CSI_N_MAX) {
				csi_n[csi_ni] = csi_n[csi_ni]*10 + ((*p) - '0');
			}
		}
	break;
	case 4:
/* #line 208 "user/ansi_parser.rl" */
	{
			if (csi_cnt == 0) csi_cnt = 1; // handle case when first arg is empty
			csi_cnt++;
			csi_ni++;
		}
	break;
	case 5:
/* #line 214 "user/ansi_parser.rl" */
	{
			csi_char = (*p);
			apars_handle_CSI(csi_leading, csi_n, csi_cnt, csi_char);
			{cs = 1;goto _again;}
		}
	break;
	case 6:
/* #line 220 "user/ansi_parser.rl" */
	{
			ansi_warn("Invalid escape sequence discarded.");
			apars_handle_badseq();
			{cs = 1;goto _again;}
		}
	break;
	case 7:
/* #line 238 "user/ansi_parser.rl" */
	{
			csi_ni = 0;

			// we reuse the CSI numeric buffer
			for(int i = 0; i < CSI_N_MAX; i++) {
				csi_n[i] = 0;
			}

			osc_bi = 0;
			osc_buffer[0] = '\0';

			inside_osc = true;

			{cs = 7;goto _again;}
		}
	break;
	case 8:
/* #line 255 "user/ansi_parser.rl" */
	{
			osc_bi = 0;
			osc_buffer[0] = '\0';
			inside_osc = true;
			{cs = 27;goto _again;}
		}
	break;
	case 9:
/* #line 262 "user/ansi_parser.rl" */
	{
			apars_handle_OSC_SetScreenSize(csi_n[0], csi_n[1]);
			inside_osc = false;
			{cs = 1;goto _again;}
		}
	break;
	case 10:
/* #line 268 "user/ansi_parser.rl" */
	{
			osc_buffer[osc_bi++] = (*p);
		}
	break;
	case 11:
/* #line 272 "user/ansi_parser.rl" */
	{
			osc_buffer[osc_bi++] = '\0';
			apars_handle_OSC_SetTitle(osc_buffer);
			inside_osc = false;
			{cs = 1;goto _again;}
		}
	break;
	case 12:
/* #line 279 "user/ansi_parser.rl" */
	{
			osc_buffer[osc_bi++] = '\0';
			apars_handle_OSC_SetButton(csi_n[0], osc_buffer);
			inside_osc = false;
			{cs = 1;goto _again;}
		}
	break;
	case 13:
/* #line 312 "user/ansi_parser.rl" */
	{
			apars_handle_hashCode((*p));
			{cs = 1;goto _again;}
		}
	break;
	case 14:
/* #line 317 "user/ansi_parser.rl" */
	{
			apars_handle_shortCode((*p));
			{cs = 1;goto _again;}
		}
	break;
	case 15:
/* #line 322 "user/ansi_parser.rl" */
	{
			apars_handle_setXCtrls((*p)); // weird control settings like 7 bit / 8 bit mode
			{cs = 1;goto _again;}
		}
	break;
	case 16:
/* #line 327 "user/ansi_parser.rl" */
	{
			// abuse the buffer for storing the leading char
			osc_buffer[0] = (*p);
			{cs = 29;goto _again;}
		}
	break;
	case 17:
/* #line 333 "user/ansi_parser.rl" */
	{
			apars_handle_characterSet(osc_buffer[0], (*p));
			{cs = 1;goto _again;}
		}
	break;
/* #line 667 "user/ansi_parser.c" */
		}
	}
	goto _again;

_again:
	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	const char *__acts = _ansi_actions + _ansi_eof_actions[cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 6:
/* #line 220 "user/ansi_parser.rl" */
	{
			ansi_warn("Invalid escape sequence discarded.");
			apars_handle_badseq();
			{cs = 1;	if ( p == pe )
		goto _test_eof;
goto _again;}
		}
	break;
/* #line 694 "user/ansi_parser.c" */
		}
	}
	}

	_out: {}
	}

/* #line 357 "user/ansi_parser.rl" */

}

// 'ESC k blah OSC_end' is a shortcut for setting title (k is defined in GNU screen as Title Definition String)
