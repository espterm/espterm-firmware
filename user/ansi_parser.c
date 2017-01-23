
/* #line 1 "user/ansi_parser.rl" */
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

/* Ragel constants block */

/* #line 188 "user/ansi_parser.c" */
static const char _ansi_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	3, 1, 4, 1, 5, 1, 6, 1, 
	7, 1, 8
};

static const char _ansi_eof_actions[] = {
	0, 0, 0, 13, 13, 0, 0
};

static const int ansi_start = 1;
static const int ansi_first_final = 5;
static const int ansi_error = 0;

static const int ansi_en_CSI_body = 3;
static const int ansi_en_main = 1;


/* #line 187 "user/ansi_parser.rl" */


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
		
/* #line 244 "user/ansi_parser.c" */
	{
	cs = ansi_start;
	}

/* #line 223 "user/ansi_parser.rl" */
	}

	// The parser
	
/* #line 254 "user/ansi_parser.c" */
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
		case 91: goto tr2;
		case 93: goto tr4;
		case 99: goto tr5;
	}
	goto tr3;
case 0:
	goto _out;
case 5:
	if ( (*p) == 27 )
		goto tr1;
	goto tr0;
case 3:
	if ( (*p) == 59 )
		goto tr9;
	if ( (*p) < 60 ) {
		if ( (*p) > 47 ) {
			if ( 48 <= (*p) && (*p) <= 57 )
				goto tr8;
		} else if ( (*p) >= 32 )
			goto tr7;
	} else if ( (*p) > 64 ) {
		if ( (*p) > 90 ) {
			if ( 97 <= (*p) && (*p) <= 122 )
				goto tr10;
		} else if ( (*p) >= 65 )
			goto tr10;
	} else
		goto tr7;
	goto tr6;
case 4:
	if ( (*p) == 59 )
		goto tr9;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr8;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto tr10;
	} else
		goto tr10;
	goto tr6;
case 6:
	goto tr6;
	}

	tr3: cs = 0; goto _again;
	tr6: cs = 0; goto f4;
	tr0: cs = 1; goto f0;
	tr1: cs = 2; goto _again;
	tr7: cs = 4; goto f5;
	tr8: cs = 4; goto f6;
	tr9: cs = 4; goto f7;
	tr2: cs = 5; goto f1;
	tr4: cs = 5; goto f2;
	tr5: cs = 5; goto f3;
	tr10: cs = 6; goto f8;

	f0: _acts = _ansi_actions + 1; goto execFuncs;
	f1: _acts = _ansi_actions + 3; goto execFuncs;
	f5: _acts = _ansi_actions + 5; goto execFuncs;
	f6: _acts = _ansi_actions + 7; goto execFuncs;
	f7: _acts = _ansi_actions + 9; goto execFuncs;
	f8: _acts = _ansi_actions + 11; goto execFuncs;
	f4: _acts = _ansi_actions + 13; goto execFuncs;
	f2: _acts = _ansi_actions + 15; goto execFuncs;
	f3: _acts = _ansi_actions + 17; goto execFuncs;

execFuncs:
	_nacts = *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 0:
/* #line 233 "user/ansi_parser.rl" */
	{
			handle_plainchar((*p));
		}
	break;
	case 1:
/* #line 240 "user/ansi_parser.rl" */
	{
			/* Reset the CSI builder */
			csi_leading = csi_char = 0;
			csi_ni = 0;

			/* Zero out digits */
			for(int i = 0; i < CSI_N_MAX; i++) {
				csi_n[i] = 0;
			}

			{cs = 3; goto _again;}
		}
	break;
	case 2:
/* #line 253 "user/ansi_parser.rl" */
	{
			csi_leading = (*p);
		}
	break;
	case 3:
/* #line 257 "user/ansi_parser.rl" */
	{
			/* x10 + digit */
			if (csi_ni < CSI_N_MAX) {
				csi_n[csi_ni] = csi_n[csi_ni]*10 + ((*p) - '0');
			}
		}
	break;
	case 4:
/* #line 264 "user/ansi_parser.rl" */
	{
			csi_ni++;
		}
	break;
	case 5:
/* #line 268 "user/ansi_parser.rl" */
	{
			csi_char = (*p);

			handle_CSI(csi_leading, csi_n, csi_char);

			{cs = 1; goto _again;}
		}
	break;
	case 6:
/* #line 276 "user/ansi_parser.rl" */
	{
			{cs = 1; goto _again;}
		}
	break;
	case 7:
/* #line 288 "user/ansi_parser.rl" */
	{
			// TODO implement OS control code parsing
			{cs = 1; goto _again;}
		}
	break;
	case 8:
/* #line 293 "user/ansi_parser.rl" */
	{
			// Reset screen
			handle_RESET_cmd();
			{cs = 1; goto _again;}
		}
	break;
/* #line 415 "user/ansi_parser.c" */
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
/* #line 276 "user/ansi_parser.rl" */
	{
			{cs = 1; goto _again;}
		}
	break;
/* #line 438 "user/ansi_parser.c" */
		}
	}
	}

	_out: {}
	}

/* #line 311 "user/ansi_parser.rl" */

}
