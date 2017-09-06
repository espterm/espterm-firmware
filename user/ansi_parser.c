
/* #line 1 "user/ansi_parser.rl" */
#include <esp8266.h>
#include "ansi_parser.h"
#include "ansi_parser_callbacks.h"
#include "ascii.h"
#include "apars_logging.h"

/* Ragel constants block */

/* #line 12 "user/ansi_parser.c" */
static const char _ansi_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	3, 1, 4, 1, 5, 1, 6, 1, 
	7, 1, 8, 1, 9, 1, 10, 1, 
	11, 1, 12, 1, 13, 1, 14, 2, 
	3, 6
};

static const char _ansi_eof_actions[] = {
	0, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 0, 0, 0, 0, 0
};

static const int ansi_start = 1;
static const int ansi_first_final = 10;
static const int ansi_error = 0;

static const int ansi_en_CSI_body = 5;
static const int ansi_en_STRCMD_body = 7;
static const int ansi_en_charsetcmd_body = 9;
static const int ansi_en_main = 1;


/* #line 11 "user/ansi_parser.rl" */


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
		ansi_warn("Parser state reset (timeout?)");
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
		
/* #line 117 "user/ansi_parser.c" */
	{
	cs = ansi_start;
	}

/* #line 91 "user/ansi_parser.rl" */

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
	if (newchar < ' ' && !inside_string) {
		switch (newchar) {
			case ESC:
				// Reset state
				cs = ansi_start;
				// now the ESC will be processed by the parser
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
				apars_handle_chs_switch(0);
				return;

			case SO:
				apars_handle_chs_switch(1);
				return;

			case BEL:
				// bel is also used to terminate OSC
				apars_handle_bel();
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
				// Discard all other control codes
				return;
		}
	} else {
		// bypass the parser for simple characters (speed-up)
		if (cs == ansi_start && newchar >= ' ') {
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
	switch( (*p) ) {
		case 7: goto tr1;
		case 27: goto tr2;
	}
	goto tr0;
case 0:
	goto _out;
case 2:
	switch( (*p) ) {
		case 32: goto tr3;
		case 35: goto tr4;
		case 37: goto tr5;
		case 80: goto tr7;
		case 88: goto tr7;
		case 91: goto tr8;
		case 107: goto tr7;
	}
	if ( (*p) < 64 ) {
		if ( (*p) < 48 ) {
			if ( 40 <= (*p) && (*p) <= 47 )
				goto tr5;
		} else if ( (*p) > 57 ) {
			if ( 60 <= (*p) && (*p) <= 62 )
				goto tr6;
		} else
			goto tr6;
	} else if ( (*p) > 92 ) {
		if ( (*p) < 97 ) {
			if ( 93 <= (*p) && (*p) <= 95 )
				goto tr7;
		} else if ( (*p) > 122 ) {
			if ( 124 <= (*p) && (*p) <= 126 )
				goto tr6;
		} else
			goto tr6;
	} else
		goto tr6;
	goto tr1;
case 3:
	if ( (*p) > 71 ) {
		if ( 76 <= (*p) && (*p) <= 78 )
			goto tr9;
	} else if ( (*p) >= 70 )
		goto tr9;
	goto tr1;
case 10:
	switch( (*p) ) {
		case 7: goto tr1;
		case 27: goto tr2;
	}
	goto tr0;
case 4:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr10;
	goto tr1;
case 5:
	switch( (*p) ) {
		case 59: goto tr13;
		case 64: goto tr14;
	}
	if ( (*p) < 60 ) {
		if ( (*p) > 47 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto tr12;
		} else if ( (*p) >= 32 )
			goto tr11;
	} else if ( (*p) > 63 ) {
		if ( (*p) > 90 ) {
			if ( 96 <= (*p) && (*p) <= 122 )
				goto tr15;
		} else if ( (*p) >= 65 )
			goto tr15;
	} else
		goto tr11;
	goto tr1;
case 6:
	if ( (*p) == 59 )
		goto tr13;
	if ( (*p) < 64 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr12;
	} else if ( (*p) > 90 ) {
		if ( 96 <= (*p) && (*p) <= 122 )
			goto tr15;
	} else
		goto tr15;
	goto tr1;
case 11:
	goto tr1;
case 12:
	if ( (*p) == 59 )
		goto tr13;
	if ( (*p) < 64 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr12;
	} else if ( (*p) > 90 ) {
		if ( 96 <= (*p) && (*p) <= 122 )
			goto tr15;
	} else
		goto tr15;
	goto tr1;
case 7:
	switch( (*p) ) {
		case 7: goto tr17;
		case 27: goto tr18;
	}
	goto tr16;
case 13:
	goto tr1;
case 8:
	if ( (*p) == 92 )
		goto tr17;
	goto tr1;
case 9:
	switch( (*p) ) {
		case 7: goto tr1;
		case 27: goto tr1;
	}
	goto tr19;
case 14:
	goto tr1;
	}

	tr1: cs = 0; goto f0;
	tr0: cs = 1; goto f1;
	tr2: cs = 2; goto _again;
	tr3: cs = 3; goto _again;
	tr4: cs = 4; goto _again;
	tr11: cs = 6; goto f8;
	tr12: cs = 6; goto f9;
	tr13: cs = 6; goto f10;
	tr16: cs = 7; goto f13;
	tr18: cs = 8; goto _again;
	tr5: cs = 10; goto f2;
	tr6: cs = 10; goto f3;
	tr7: cs = 10; goto f4;
	tr8: cs = 10; goto f5;
	tr9: cs = 10; goto f6;
	tr10: cs = 10; goto f7;
	tr15: cs = 11; goto f12;
	tr14: cs = 12; goto f11;
	tr17: cs = 13; goto f14;
	tr19: cs = 14; goto f15;

	f0: _acts = _ansi_actions + 1; goto execFuncs;
	f1: _acts = _ansi_actions + 3; goto execFuncs;
	f5: _acts = _ansi_actions + 5; goto execFuncs;
	f8: _acts = _ansi_actions + 7; goto execFuncs;
	f9: _acts = _ansi_actions + 9; goto execFuncs;
	f10: _acts = _ansi_actions + 11; goto execFuncs;
	f12: _acts = _ansi_actions + 13; goto execFuncs;
	f4: _acts = _ansi_actions + 15; goto execFuncs;
	f13: _acts = _ansi_actions + 17; goto execFuncs;
	f14: _acts = _ansi_actions + 19; goto execFuncs;
	f7: _acts = _ansi_actions + 21; goto execFuncs;
	f3: _acts = _ansi_actions + 23; goto execFuncs;
	f6: _acts = _ansi_actions + 25; goto execFuncs;
	f2: _acts = _ansi_actions + 27; goto execFuncs;
	f15: _acts = _ansi_actions + 29; goto execFuncs;
	f11: _acts = _ansi_actions + 31; goto execFuncs;

execFuncs:
	_nacts = *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 0:
/* #line 178 "user/ansi_parser.rl" */
	{
			ansi_warn("Parser error.");
			apars_show_context();
			inside_string = false; // no longer in string, for sure
			{cs = 1;goto _again;}
		}
	break;
	case 1:
/* #line 187 "user/ansi_parser.rl" */
	{
			if ((*p) != 0) {
				apars_handle_plainchar((*p));
			}
		}
	break;
	case 2:
/* #line 195 "user/ansi_parser.rl" */
	{
			// Reset the CSI builder
			leadchar = NUL;
			arg_ni = 0;
			arg_cnt = 0;

			// Zero out digits
			for(int i = 0; i < CSI_N_MAX; i++) {
				arg[i] = 0;
			}

			{cs = 5;goto _again;}
		}
	break;
	case 3:
/* #line 209 "user/ansi_parser.rl" */
	{
			leadchar = (*p);
		}
	break;
	case 4:
/* #line 213 "user/ansi_parser.rl" */
	{
			if (arg_cnt == 0) arg_cnt = 1;
			// x10 + digit
			if (arg_ni < CSI_N_MAX) {
				arg[arg_ni] = arg[arg_ni]*10 + ((*p) - '0');
			}
		}
	break;
	case 5:
/* #line 221 "user/ansi_parser.rl" */
	{
			if (arg_cnt == 0) arg_cnt = 1; // handle case when first arg is empty
			arg_cnt++;
			arg_ni++;
		}
	break;
	case 6:
/* #line 227 "user/ansi_parser.rl" */
	{
			apars_handle_csi(leadchar, arg, arg_cnt, (*p));
			{cs = 1;goto _again;}
		}
	break;
	case 7:
/* #line 238 "user/ansi_parser.rl" */
	{
			leadchar = (*p);
			str_ni = 0;
			string_buffer[0] = '\0';
			inside_string = true;
			{cs = 7;goto _again;}
		}
	break;
	case 8:
/* #line 246 "user/ansi_parser.rl" */
	{
			string_buffer[str_ni++] = (*p);
		}
	break;
	case 9:
/* #line 250 "user/ansi_parser.rl" */
	{
			inside_string = false;
			string_buffer[str_ni++] = '\0';
			apars_handle_string_cmd(leadchar, string_buffer);
			{cs = 1;goto _again;}
		}
	break;
	case 10:
/* #line 263 "user/ansi_parser.rl" */
	{
			apars_handle_hash_cmd((*p));
			{cs = 1;goto _again;}
		}
	break;
	case 11:
/* #line 268 "user/ansi_parser.rl" */
	{
			apars_handle_short_cmd((*p));
			{cs = 1;goto _again;}
		}
	break;
	case 12:
/* #line 273 "user/ansi_parser.rl" */
	{
			apars_handle_space_cmd((*p));
			{cs = 1;goto _again;}
		}
	break;
	case 13:
/* #line 280 "user/ansi_parser.rl" */
	{
			leadchar = (*p);
			{cs = 9;goto _again;}
		}
	break;
	case 14:
/* #line 285 "user/ansi_parser.rl" */
	{
			apars_handle_chs_designate(leadchar, (*p));
			{cs = 1;goto _again;}
		}
	break;
/* #line 503 "user/ansi_parser.c" */
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
	case 0:
/* #line 178 "user/ansi_parser.rl" */
	{
			ansi_warn("Parser error.");
			apars_show_context();
			inside_string = false; // no longer in string, for sure
			{cs = 1;	if ( p == pe )
		goto _test_eof;
goto _again;}
		}
	break;
/* #line 531 "user/ansi_parser.c" */
		}
	}
	}

	_out: {}
	}

/* #line 308 "user/ansi_parser.rl" */

}
