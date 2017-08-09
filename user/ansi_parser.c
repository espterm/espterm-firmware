
/* #line 1 "user/ansi_parser.rl" */
#include <esp8266.h>
#include "ansi_parser.h"
#include "screen.h"

/* Ragel constants block */

/* #line 10 "user/ansi_parser.c" */
static const char _ansi_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	3, 1, 4, 1, 5, 1, 6, 1, 
	7, 1, 8, 1, 9, 1, 10, 1, 
	11, 1, 12, 1, 13, 1, 14, 2, 
	10, 11, 2, 10, 12
};

static const char _ansi_eof_actions[] = {
	0, 13, 13, 13, 13, 13, 13, 13, 
	13, 13, 13, 13, 13, 13, 13, 13, 
	13, 13, 13, 13, 13, 13, 13, 13, 
	13, 13, 13, 13, 0, 0, 0, 0, 
	0, 0, 0
};

static const int ansi_start = 1;
static const int ansi_first_final = 28;
static const int ansi_error = 0;

static const int ansi_en_CSI_body = 4;
static const int ansi_en_OSC_body = 6;
static const int ansi_en_TITLE_body = 26;
static const int ansi_en_main = 1;


/* #line 9 "user/ansi_parser.rl" */


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
		
/* #line 86 "user/ansi_parser.c" */
	{
	cs = ansi_start;
	}

/* #line 57 "user/ansi_parser.rl" */
	}

	// schedule state reset
	if (termconf->parser_tout_ms > 0) {
		os_timer_disarm(&resetTim);
		os_timer_setfn(&resetTim, resetParserCb, NULL);
		os_timer_arm(&resetTim, termconf->parser_tout_ms, 0);
	}

	// The parser
	
/* #line 103 "user/ansi_parser.c" */
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
		case 35: goto tr3;
		case 91: goto tr5;
		case 93: goto tr6;
		case 107: goto tr7;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr4;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto tr4;
	} else
		goto tr4;
	goto tr2;
case 0:
	goto _out;
case 3:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr8;
	goto tr2;
case 28:
	if ( (*p) == 27 )
		goto tr1;
	goto tr0;
case 4:
	if ( (*p) == 59 )
		goto tr11;
	if ( (*p) < 60 ) {
		if ( (*p) > 47 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto tr10;
		} else if ( (*p) >= 32 )
			goto tr9;
	} else if ( (*p) > 64 ) {
		if ( (*p) > 90 ) {
			if ( 97 <= (*p) && (*p) <= 122 )
				goto tr12;
		} else if ( (*p) >= 65 )
			goto tr12;
	} else
		goto tr9;
	goto tr2;
case 5:
	if ( (*p) == 59 )
		goto tr11;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr10;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto tr12;
	} else
		goto tr12;
	goto tr2;
case 29:
	goto tr2;
case 6:
	switch( (*p) ) {
		case 48: goto tr13;
		case 66: goto tr14;
		case 84: goto tr15;
		case 87: goto tr16;
	}
	goto tr2;
case 7:
	if ( (*p) == 59 )
		goto tr17;
	goto tr2;
case 8:
	switch( (*p) ) {
		case 7: goto tr19;
		case 27: goto tr20;
	}
	goto tr18;
case 30:
	switch( (*p) ) {
		case 7: goto tr19;
		case 27: goto tr20;
	}
	goto tr18;
case 9:
	if ( (*p) == 92 )
		goto tr21;
	goto tr2;
case 31:
	goto tr2;
case 10:
	if ( (*p) == 84 )
		goto tr22;
	goto tr2;
case 11:
	if ( (*p) == 78 )
		goto tr23;
	goto tr2;
case 12:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr24;
	goto tr2;
case 13:
	if ( (*p) == 61 )
		goto tr25;
	goto tr2;
case 14:
	switch( (*p) ) {
		case 7: goto tr27;
		case 27: goto tr28;
	}
	goto tr26;
case 32:
	switch( (*p) ) {
		case 7: goto tr27;
		case 27: goto tr28;
	}
	goto tr26;
case 15:
	if ( (*p) == 92 )
		goto tr29;
	goto tr2;
case 16:
	if ( (*p) == 73 )
		goto tr30;
	goto tr2;
case 17:
	if ( (*p) == 84 )
		goto tr31;
	goto tr2;
case 18:
	if ( (*p) == 76 )
		goto tr32;
	goto tr2;
case 19:
	if ( (*p) == 69 )
		goto tr33;
	goto tr2;
case 20:
	if ( (*p) == 61 )
		goto tr34;
	goto tr2;
case 21:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr35;
	goto tr2;
case 22:
	if ( (*p) == 59 )
		goto tr36;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr35;
	goto tr2;
case 23:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr37;
	goto tr2;
case 24:
	switch( (*p) ) {
		case 7: goto tr38;
		case 27: goto tr39;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr37;
	goto tr2;
case 25:
	if ( (*p) == 92 )
		goto tr38;
	goto tr2;
case 26:
	switch( (*p) ) {
		case 7: goto tr41;
		case 27: goto tr42;
	}
	goto tr40;
case 33:
	switch( (*p) ) {
		case 7: goto tr41;
		case 27: goto tr42;
	}
	goto tr40;
case 27:
	if ( (*p) == 92 )
		goto tr43;
	goto tr2;
case 34:
	goto tr2;
	}

	tr2: cs = 0; goto f0;
	tr0: cs = 1; goto f1;
	tr1: cs = 2; goto _again;
	tr3: cs = 3; goto _again;
	tr9: cs = 5; goto f7;
	tr10: cs = 5; goto f8;
	tr11: cs = 5; goto f9;
	tr13: cs = 7; goto _again;
	tr17: cs = 8; goto _again;
	tr18: cs = 8; goto f11;
	tr20: cs = 9; goto _again;
	tr14: cs = 10; goto _again;
	tr22: cs = 11; goto _again;
	tr23: cs = 12; goto _again;
	tr24: cs = 13; goto f8;
	tr25: cs = 14; goto _again;
	tr26: cs = 14; goto f11;
	tr28: cs = 15; goto _again;
	tr15: cs = 16; goto _again;
	tr30: cs = 17; goto _again;
	tr31: cs = 18; goto _again;
	tr32: cs = 19; goto _again;
	tr33: cs = 20; goto _again;
	tr16: cs = 21; goto _again;
	tr35: cs = 22; goto f8;
	tr36: cs = 23; goto f9;
	tr37: cs = 24; goto f8;
	tr39: cs = 25; goto _again;
	tr40: cs = 26; goto f11;
	tr42: cs = 27; goto _again;
	tr4: cs = 28; goto f2;
	tr5: cs = 28; goto f3;
	tr6: cs = 28; goto f4;
	tr7: cs = 28; goto f5;
	tr8: cs = 28; goto f6;
	tr12: cs = 29; goto f10;
	tr19: cs = 30; goto f12;
	tr34: cs = 31; goto f5;
	tr21: cs = 31; goto f13;
	tr29: cs = 31; goto f15;
	tr38: cs = 31; goto f16;
	tr27: cs = 32; goto f14;
	tr41: cs = 33; goto f12;
	tr43: cs = 34; goto f13;

	f1: _acts = _ansi_actions + 1; goto execFuncs;
	f3: _acts = _ansi_actions + 3; goto execFuncs;
	f7: _acts = _ansi_actions + 5; goto execFuncs;
	f8: _acts = _ansi_actions + 7; goto execFuncs;
	f9: _acts = _ansi_actions + 9; goto execFuncs;
	f10: _acts = _ansi_actions + 11; goto execFuncs;
	f0: _acts = _ansi_actions + 13; goto execFuncs;
	f4: _acts = _ansi_actions + 15; goto execFuncs;
	f5: _acts = _ansi_actions + 17; goto execFuncs;
	f16: _acts = _ansi_actions + 19; goto execFuncs;
	f11: _acts = _ansi_actions + 21; goto execFuncs;
	f13: _acts = _ansi_actions + 23; goto execFuncs;
	f15: _acts = _ansi_actions + 25; goto execFuncs;
	f6: _acts = _ansi_actions + 27; goto execFuncs;
	f2: _acts = _ansi_actions + 29; goto execFuncs;
	f12: _acts = _ansi_actions + 31; goto execFuncs;
	f14: _acts = _ansi_actions + 34; goto execFuncs;

execFuncs:
	_nacts = *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 0:
/* #line 76 "user/ansi_parser.rl" */
	{
			apars_handle_plainchar((*p));
		}
	break;
	case 1:
/* #line 83 "user/ansi_parser.rl" */
	{
			// Reset the CSI builder
			csi_leading = csi_char = 0;
			csi_ni = 0;

			// Zero out digits
			for(int i = 0; i < CSI_N_MAX; i++) {
				csi_n[i] = 0;
			}

			{cs = 4;goto _again;}
		}
	break;
	case 2:
/* #line 96 "user/ansi_parser.rl" */
	{
			csi_leading = (*p);
		}
	break;
	case 3:
/* #line 100 "user/ansi_parser.rl" */
	{
			// x10 + digit
			if (csi_ni < CSI_N_MAX) {
				csi_n[csi_ni] = csi_n[csi_ni]*10 + ((*p) - '0');
			}
		}
	break;
	case 4:
/* #line 107 "user/ansi_parser.rl" */
	{
			csi_ni++;
		}
	break;
	case 5:
/* #line 111 "user/ansi_parser.rl" */
	{
			csi_char = (*p);

			apars_handle_CSI(csi_leading, csi_n, csi_char);

			{cs = 1;goto _again;}
		}
	break;
	case 6:
/* #line 119 "user/ansi_parser.rl" */
	{
			apars_handle_badseq();
			{cs = 1;goto _again;}
		}
	break;
	case 7:
/* #line 136 "user/ansi_parser.rl" */
	{
			csi_ni = 0;

			// we reuse the CSI numeric buffer
			for(int i = 0; i < CSI_N_MAX; i++) {
				csi_n[i] = 0;
			}

			osc_bi = 0;
			osc_buffer[0] = '\0';

			{cs = 6;goto _again;}
		}
	break;
	case 8:
/* #line 151 "user/ansi_parser.rl" */
	{
			osc_bi = 0;
			osc_buffer[0] = '\0';
			{cs = 26;goto _again;}
		}
	break;
	case 9:
/* #line 157 "user/ansi_parser.rl" */
	{
			apars_handle_OSC_SetScreenSize(csi_n[0], csi_n[1]);
			{cs = 1;goto _again;}
		}
	break;
	case 10:
/* #line 162 "user/ansi_parser.rl" */
	{
			osc_buffer[osc_bi++] = (*p);
		}
	break;
	case 11:
/* #line 166 "user/ansi_parser.rl" */
	{
			osc_buffer[osc_bi++] = '\0';
			apars_handle_OSC_SetTitle(osc_buffer);
			{cs = 1;goto _again;}
		}
	break;
	case 12:
/* #line 172 "user/ansi_parser.rl" */
	{
			osc_buffer[osc_bi++] = '\0';
			apars_handle_OSC_SetButton(csi_n[0], osc_buffer);
			{cs = 1;goto _again;}
		}
	break;
	case 13:
/* #line 204 "user/ansi_parser.rl" */
	{
			apars_handle_hashCode((*p));
			{cs = 1;goto _again;}
		}
	break;
	case 14:
/* #line 209 "user/ansi_parser.rl" */
	{
			apars_handle_shortCode((*p));
			{cs = 1;goto _again;}
		}
	break;
/* #line 497 "user/ansi_parser.c" */
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
/* #line 119 "user/ansi_parser.rl" */
	{
			apars_handle_badseq();
			{cs = 1;	if ( p == pe )
		goto _test_eof;
goto _again;}
		}
	break;
/* #line 523 "user/ansi_parser.c" */
		}
	}
	}

	_out: {}
	}

/* #line 229 "user/ansi_parser.rl" */

}

// 'ESC k blah OSC_end' is a shortcut for setting title (k is defined in GNU screen as Title Definition String)
