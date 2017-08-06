
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
	11, 1, 12, 1, 13, 2, 9, 10, 
	2, 9, 11
};

static const char _ansi_eof_actions[] = {
	0, 13, 13, 13, 13, 13, 13, 13, 
	13, 13, 13, 13, 13, 13, 13, 13, 
	13, 13, 13, 13, 13, 13, 13, 13, 
	13, 0, 0, 0, 0, 0
};

static const int ansi_start = 1;
static const int ansi_first_final = 25;
static const int ansi_error = 0;

static const int ansi_en_CSI_body = 4;
static const int ansi_en_OSC_body = 6;
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
		
/* #line 84 "user/ansi_parser.c" */
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
	
/* #line 101 "user/ansi_parser.c" */
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
		goto tr7;
	goto tr2;
case 25:
	if ( (*p) == 27 )
		goto tr1;
	goto tr0;
case 4:
	if ( (*p) == 59 )
		goto tr10;
	if ( (*p) < 60 ) {
		if ( (*p) > 47 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto tr9;
		} else if ( (*p) >= 32 )
			goto tr8;
	} else if ( (*p) > 64 ) {
		if ( (*p) > 90 ) {
			if ( 97 <= (*p) && (*p) <= 122 )
				goto tr11;
		} else if ( (*p) >= 65 )
			goto tr11;
	} else
		goto tr8;
	goto tr2;
case 5:
	if ( (*p) == 59 )
		goto tr10;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr9;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto tr11;
	} else
		goto tr11;
	goto tr2;
case 26:
	goto tr2;
case 6:
	switch( (*p) ) {
		case 66: goto tr12;
		case 84: goto tr13;
		case 87: goto tr14;
	}
	goto tr2;
case 7:
	if ( (*p) == 84 )
		goto tr15;
	goto tr2;
case 8:
	if ( (*p) == 78 )
		goto tr16;
	goto tr2;
case 9:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr17;
	goto tr2;
case 10:
	if ( (*p) == 61 )
		goto tr18;
	goto tr2;
case 11:
	switch( (*p) ) {
		case 7: goto tr20;
		case 27: goto tr21;
	}
	goto tr19;
case 27:
	switch( (*p) ) {
		case 7: goto tr20;
		case 27: goto tr21;
	}
	goto tr19;
case 12:
	if ( (*p) == 92 )
		goto tr22;
	goto tr2;
case 28:
	goto tr2;
case 13:
	if ( (*p) == 73 )
		goto tr23;
	goto tr2;
case 14:
	if ( (*p) == 84 )
		goto tr24;
	goto tr2;
case 15:
	if ( (*p) == 76 )
		goto tr25;
	goto tr2;
case 16:
	if ( (*p) == 69 )
		goto tr26;
	goto tr2;
case 17:
	if ( (*p) == 61 )
		goto tr27;
	goto tr2;
case 18:
	switch( (*p) ) {
		case 7: goto tr29;
		case 27: goto tr30;
	}
	goto tr28;
case 29:
	switch( (*p) ) {
		case 7: goto tr29;
		case 27: goto tr30;
	}
	goto tr28;
case 19:
	if ( (*p) == 92 )
		goto tr31;
	goto tr2;
case 20:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr32;
	goto tr2;
case 21:
	if ( (*p) == 59 )
		goto tr33;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr32;
	goto tr2;
case 22:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr34;
	goto tr2;
case 23:
	switch( (*p) ) {
		case 7: goto tr35;
		case 27: goto tr36;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr34;
	goto tr2;
case 24:
	if ( (*p) == 92 )
		goto tr35;
	goto tr2;
	}

	tr2: cs = 0; goto f0;
	tr0: cs = 1; goto f1;
	tr1: cs = 2; goto _again;
	tr3: cs = 3; goto _again;
	tr8: cs = 5; goto f6;
	tr9: cs = 5; goto f7;
	tr10: cs = 5; goto f8;
	tr12: cs = 7; goto _again;
	tr15: cs = 8; goto _again;
	tr16: cs = 9; goto _again;
	tr17: cs = 10; goto f7;
	tr18: cs = 11; goto _again;
	tr19: cs = 11; goto f10;
	tr21: cs = 12; goto _again;
	tr13: cs = 13; goto _again;
	tr23: cs = 14; goto _again;
	tr24: cs = 15; goto _again;
	tr25: cs = 16; goto _again;
	tr26: cs = 17; goto _again;
	tr27: cs = 18; goto _again;
	tr28: cs = 18; goto f10;
	tr30: cs = 19; goto _again;
	tr14: cs = 20; goto _again;
	tr32: cs = 21; goto f7;
	tr33: cs = 22; goto f8;
	tr34: cs = 23; goto f7;
	tr36: cs = 24; goto _again;
	tr4: cs = 25; goto f2;
	tr5: cs = 25; goto f3;
	tr6: cs = 25; goto f4;
	tr7: cs = 25; goto f5;
	tr11: cs = 26; goto f9;
	tr20: cs = 27; goto f11;
	tr22: cs = 28; goto f12;
	tr31: cs = 28; goto f14;
	tr35: cs = 28; goto f15;
	tr29: cs = 29; goto f13;

	f1: _acts = _ansi_actions + 1; goto execFuncs;
	f3: _acts = _ansi_actions + 3; goto execFuncs;
	f6: _acts = _ansi_actions + 5; goto execFuncs;
	f7: _acts = _ansi_actions + 7; goto execFuncs;
	f8: _acts = _ansi_actions + 9; goto execFuncs;
	f9: _acts = _ansi_actions + 11; goto execFuncs;
	f0: _acts = _ansi_actions + 13; goto execFuncs;
	f4: _acts = _ansi_actions + 15; goto execFuncs;
	f15: _acts = _ansi_actions + 17; goto execFuncs;
	f10: _acts = _ansi_actions + 19; goto execFuncs;
	f14: _acts = _ansi_actions + 21; goto execFuncs;
	f12: _acts = _ansi_actions + 23; goto execFuncs;
	f5: _acts = _ansi_actions + 25; goto execFuncs;
	f2: _acts = _ansi_actions + 27; goto execFuncs;
	f13: _acts = _ansi_actions + 29; goto execFuncs;
	f11: _acts = _ansi_actions + 32; goto execFuncs;

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
/* #line 150 "user/ansi_parser.rl" */
	{
			apars_handle_OSC_SetScreenSize(csi_n[0], csi_n[1]);
			{cs = 1;goto _again;}
		}
	break;
	case 9:
/* #line 155 "user/ansi_parser.rl" */
	{
			osc_buffer[osc_bi++] = (*p);
		}
	break;
	case 10:
/* #line 159 "user/ansi_parser.rl" */
	{
			osc_buffer[osc_bi++] = '\0';
			apars_handle_OSC_SetTitle(osc_buffer);
			{cs = 1;goto _again;}
		}
	break;
	case 11:
/* #line 165 "user/ansi_parser.rl" */
	{
			osc_buffer[osc_bi++] = '\0';
			apars_handle_OSC_SetButton(csi_n[0], osc_buffer);
			{cs = 1;goto _again;}
		}
	break;
	case 12:
/* #line 193 "user/ansi_parser.rl" */
	{
			apars_handle_hashCode((*p));
			{cs = 1;goto _again;}
		}
	break;
	case 13:
/* #line 198 "user/ansi_parser.rl" */
	{
			apars_handle_shortCode((*p));
			{cs = 1;goto _again;}
		}
	break;
/* #line 455 "user/ansi_parser.c" */
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
/* #line 481 "user/ansi_parser.c" */
		}
	}
	}

	_out: {}
	}

/* #line 217 "user/ansi_parser.rl" */

}
