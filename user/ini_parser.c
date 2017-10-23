
/* #line 1 "user/ini_parser.rl" */

/* Ragel constants block */
#include "ini_parser.h"

// Ragel setup

/* #line 10 "user/ini_parser.c" */
static const char _ini_actions[] ESP_CONST_DATA = {
	0, 1, 1, 1, 2, 1, 3, 1, 
	4, 1, 5, 1, 6, 1, 7, 1, 
	8, 1, 9, 1, 10, 1, 11, 1, 
	13, 2, 0, 4, 2, 12, 4
};

static const char _ini_eof_actions[] ESP_CONST_DATA = {
	0, 23, 5, 5, 15, 15, 15, 15, 
	19, 19, 0, 0, 0, 0, 0, 0, 
	0
};

static const int ini_start = 1;
static const int ini_first_final = 12;
static const int ini_error = 0;

static const int ini_en_section = 2;
static const int ini_en_keyvalue = 4;
static const int ini_en_comment = 8;
static const int ini_en_discard2eol = 10;
static const int ini_en_main = 1;


/* #line 10 "user/ini_parser.rl" */


// Persistent state
static int8_t cs = -1; //!< Ragel's Current State variable
static uint32_t buff_i = 0; //!< Write pointer for the buffers
static char value_quote = 0; //!< Quote character of the currently collected value
static bool value_nextesc = false; //!< Next character is escaped, trated specially, and if quote, as literal quote character
static IniParserCallback keyCallback = NULL; //!< Currently assigned callback
static void *userdata = NULL; //!< Currently assigned user data for the callback

// Buffers
static char keybuf[INI_KEY_MAX];
static char secbuf[INI_KEY_MAX];
static char valbuf[INI_VALUE_MAX];

// See header for doxygen!

void ICACHE_FLASH_ATTR
ini_parse_reset_partial(void)
{
	buff_i = 0;
	value_quote = 0;
	value_nextesc = false;
}

void ICACHE_FLASH_ATTR
ini_parse_reset(void)
{
	ini_parse_reset_partial();
	keybuf[0] = secbuf[0] = valbuf[0] = 0;
	
/* #line 67 "user/ini_parser.c" */
	{
	cs = ini_start;
	}

/* #line 41 "user/ini_parser.rl" */
}

void ICACHE_FLASH_ATTR
ini_parser_error(const char* msg)
{
	ini_error("Parser error: %s", msg);
	ini_parse_reset_partial();
}


void ICACHE_FLASH_ATTR
ini_parse_begin(IniParserCallback callback, void *userData)
{
	keyCallback = callback;
	userdata = userData;
	ini_parse_reset();
}


void ICACHE_FLASH_ATTR
*ini_parse_end(void)
{
	ini_parse("\n", 1);
	if (keyCallback) {
		keyCallback = NULL;
	}

	void *ud = userdata;
	userdata = NULL;
	return ud;
}


void ICACHE_FLASH_ATTR
ini_parse_file(const char *text, size_t len, IniParserCallback callback, void *userData)
{
	ini_parse_begin(callback, userData);
	ini_parse(text, len);
	ini_parse_end();
}

static void ICACHE_FLASH_ATTR
rtrim_buf(char *buf, int32_t end)
{
	if (end > 0) {
		while ((uint8_t)buf[--end] < 33);
		end++; // go past the last character
	}

	buf[end] = 0;
}


void ICACHE_FLASH_ATTR
ini_parse(const char *newstr, size_t len)
{
	int32_t i;
	char c;
	bool isnl;
	bool isquot;

	// Load new data to Ragel vars
	const uint8_t *p;
	const uint8_t *eof;
	const uint8_t *pe;

	if (len == 0) while(newstr[++len] != 0); // alternative to strlen

	p = (const uint8_t *) newstr;
	eof = NULL;
	pe = (const uint8_t *) (newstr + len);

	// Init Ragel on the first run
	if (cs == -1) {
		ini_parse_reset();
	}

	// The parser
	
/* #line 152 "user/ini_parser.c" */
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
		case 32u: goto tr1;
		case 35u: goto tr3;
		case 58u: goto tr0;
		case 59u: goto tr3;
		case 61u: goto tr0;
		case 91u: goto tr4;
	}
	if ( (*p) < 9u ) {
		if ( (*p) <= 8u )
			goto tr0;
	} else if ( (*p) > 13u ) {
		if ( 14u <= (*p) && (*p) <= 31u )
			goto tr0;
	} else
		goto tr1;
	goto tr2;
case 0:
	goto _out;
case 12:
	goto tr0;
case 2:
	switch( (*p) ) {
		case 9u: goto tr6;
		case 32u: goto tr6;
		case 93u: goto tr5;
	}
	if ( (*p) <= 31u )
		goto tr5;
	goto tr7;
case 3:
	if ( (*p) == 93u )
		goto tr8;
	if ( (*p) > 8u ) {
		if ( 10u <= (*p) && (*p) <= 31u )
			goto tr5;
	} else
		goto tr5;
	goto tr7;
case 13:
	goto tr5;
case 4:
	switch( (*p) ) {
		case 10u: goto tr10;
		case 58u: goto tr11;
		case 61u: goto tr11;
	}
	goto tr9;
case 5:
	switch( (*p) ) {
		case 9u: goto tr13;
		case 10u: goto tr14;
		case 13u: goto tr15;
		case 32u: goto tr13;
	}
	goto tr12;
case 6:
	switch( (*p) ) {
		case 10u: goto tr14;
		case 13u: goto tr15;
	}
	goto tr12;
case 14:
	goto tr10;
case 7:
	if ( (*p) == 10u )
		goto tr14;
	goto tr10;
case 8:
	switch( (*p) ) {
		case 10u: goto tr17;
		case 13u: goto tr18;
	}
	goto tr16;
case 15:
	goto tr19;
case 9:
	if ( (*p) == 10u )
		goto tr17;
	goto tr19;
case 10:
	switch( (*p) ) {
		case 10u: goto tr21;
		case 13u: goto tr22;
	}
	goto tr20;
case 16:
	goto tr23;
case 11:
	if ( (*p) == 10u )
		goto tr21;
	goto tr23;
	}

	tr23: cs = 0; goto _again;
	tr0: cs = 0; goto f0;
	tr5: cs = 0; goto f4;
	tr10: cs = 0; goto f7;
	tr19: cs = 0; goto f11;
	tr1: cs = 1; goto _again;
	tr6: cs = 2; goto _again;
	tr7: cs = 3; goto f5;
	tr9: cs = 4; goto f8;
	tr13: cs = 5; goto _again;
	tr11: cs = 5; goto f9;
	tr12: cs = 6; goto f10;
	tr15: cs = 7; goto _again;
	tr16: cs = 8; goto _again;
	tr18: cs = 9; goto _again;
	tr20: cs = 10; goto _again;
	tr22: cs = 11; goto _again;
	tr2: cs = 12; goto f1;
	tr3: cs = 12; goto f2;
	tr4: cs = 12; goto f3;
	tr8: cs = 13; goto f6;
	tr14: cs = 14; goto f10;
	tr17: cs = 15; goto f12;
	tr21: cs = 16; goto f13;

	f5: _acts = _ini_actions + 1; goto execFuncs;
	f6: _acts = _ini_actions + 3; goto execFuncs;
	f4: _acts = _ini_actions + 5; goto execFuncs;
	f1: _acts = _ini_actions + 7; goto execFuncs;
	f8: _acts = _ini_actions + 9; goto execFuncs;
	f9: _acts = _ini_actions + 11; goto execFuncs;
	f10: _acts = _ini_actions + 13; goto execFuncs;
	f7: _acts = _ini_actions + 15; goto execFuncs;
	f12: _acts = _ini_actions + 17; goto execFuncs;
	f11: _acts = _ini_actions + 19; goto execFuncs;
	f13: _acts = _ini_actions + 21; goto execFuncs;
	f0: _acts = _ini_actions + 23; goto execFuncs;
	f3: _acts = _ini_actions + 25; goto execFuncs;
	f2: _acts = _ini_actions + 28; goto execFuncs;

execFuncs:
	_nacts = *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 0:
/* #line 130 "user/ini_parser.rl" */
	{
			buff_i = 0;
			{cs = 2;goto _again;}
		}
	break;
	case 1:
/* #line 135 "user/ini_parser.rl" */
	{
			if (buff_i >= INI_KEY_MAX) {
				ini_parser_error("Section name too long");
				{cs = 10;goto _again;}
			}
			keybuf[buff_i++] = (*p);
		}
	break;
	case 2:
/* #line 143 "user/ini_parser.rl" */
	{
			// we need a separate buffer for the result, otherwise a failed
			// partial parse would corrupt the section string
			rtrim_buf(keybuf, buff_i);
			for (i = 0; (c = keybuf[i]) != 0; i++) secbuf[i] = c;
			secbuf[i] = 0;
			{cs = 1;goto _again;}
		}
	break;
	case 3:
/* #line 155 "user/ini_parser.rl" */
	{
				ini_parser_error("Syntax error in [section]");
				if((*p) == '\n') {cs = 1;goto _again;} else {cs = 10;goto _again;}
			}
	break;
	case 4:
/* #line 162 "user/ini_parser.rl" */
	{
			buff_i = 0;
			keybuf[buff_i++] = (*p); // add the first char
			{cs = 4;goto _again;}
		}
	break;
	case 5:
/* #line 168 "user/ini_parser.rl" */
	{
			if (buff_i >= INI_KEY_MAX) {
				ini_parser_error("Key too long");
				{cs = 10;goto _again;}
			}
			keybuf[buff_i++] = (*p);
		}
	break;
	case 6:
/* #line 176 "user/ini_parser.rl" */
	{
			rtrim_buf(keybuf, buff_i);

			// --- Value begin ---
			buff_i = 0;
			value_quote = 0;
			value_nextesc = false;
		}
	break;
	case 7:
/* #line 185 "user/ini_parser.rl" */
	{
			isnl = ((*p) == '\r' || (*p) == '\n');
			isquot = ((*p) == '\'' || (*p) == '"');

			// detect our starting quote
			if (isquot && !value_nextesc && buff_i == 0 && value_quote == 0) {
				value_quote = (*p);
				goto valueCharDone;
			}

			if (buff_i >= INI_VALUE_MAX) {
				ini_parser_error("Value too long");
				{cs = 10;goto _again;}
			}

			// end of string - clean up and report
			if ((!value_nextesc && (*p) == value_quote) || isnl) {
				if (isnl && value_quote) {
					ini_parser_error("Unterminated string");
					{cs = 1;goto _again;}
				}

				// unquoted: trim from the end
				if (!value_quote) {
					rtrim_buf(valbuf, buff_i);
				} else {
					valbuf[buff_i] = 0;
				}

				if (keyCallback) {
					keyCallback(secbuf, keybuf, valbuf, userdata);
				}

				// we don't want to discard to eol if the string was terminated by eol
				// - would delete the next line

				if (isnl) {cs = 1;goto _again;} else {cs = 10;goto _again;}
			}

			c = (*p);
			// escape...
			if (value_nextesc) {
				if ((*p) == 'n') c = '\n';
				else if ((*p) == 'r') c = '\r';
				else if ((*p) == 't') c = '\t';
				else if ((*p) == 'e') c = '\033';
			}

			// collecting characters...
			if (value_nextesc || (*p) != '\\') { // is quoted, or is not a quoting backslash - literal character
				valbuf[buff_i++] = c;
			}

			value_nextesc = (!value_nextesc && (*p) == '\\');
valueCharDone:;
		}
	break;
	case 8:
/* #line 247 "user/ini_parser.rl" */
	{
				ini_parser_error("Syntax error in key=value");
				if((*p) == '\n') {cs = 1;goto _again;} else {cs = 10;goto _again;}
			}
	break;
	case 9:
/* #line 257 "user/ini_parser.rl" */
	{ {cs = 1;goto _again;} }
	break;
	case 10:
/* #line 258 "user/ini_parser.rl" */
	{
				ini_parser_error("Syntax error in comment");
				if((*p) == '\n') {cs = 1;goto _again;} else {cs = 10;goto _again;}
			}
	break;
	case 11:
/* #line 265 "user/ini_parser.rl" */
	{ {cs = 1;goto _again;} }
	break;
	case 12:
/* #line 273 "user/ini_parser.rl" */
	{ {cs = 8;goto _again;} }
	break;
	case 13:
/* #line 276 "user/ini_parser.rl" */
	{
				ini_parser_error("Syntax error in root");
				{cs = 10;goto _again;}
			}
	break;
/* #line 458 "user/ini_parser.c" */
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
	const char *__acts = _ini_actions + _ini_eof_actions[cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 3:
/* #line 155 "user/ini_parser.rl" */
	{
				ini_parser_error("Syntax error in [section]");
				if((*p) == '\n') {cs = 1;	if ( p == pe )
		goto _test_eof;
goto _again;} else {cs = 10;	if ( p == pe )
		goto _test_eof;
goto _again;}
			}
	break;
	case 8:
/* #line 247 "user/ini_parser.rl" */
	{
				ini_parser_error("Syntax error in key=value");
				if((*p) == '\n') {cs = 1;	if ( p == pe )
		goto _test_eof;
goto _again;} else {cs = 10;	if ( p == pe )
		goto _test_eof;
goto _again;}
			}
	break;
	case 10:
/* #line 258 "user/ini_parser.rl" */
	{
				ini_parser_error("Syntax error in comment");
				if((*p) == '\n') {cs = 1;	if ( p == pe )
		goto _test_eof;
goto _again;} else {cs = 10;	if ( p == pe )
		goto _test_eof;
goto _again;}
			}
	break;
	case 13:
/* #line 276 "user/ini_parser.rl" */
	{
				ini_parser_error("Syntax error in root");
				{cs = 10;	if ( p == pe )
		goto _test_eof;
goto _again;}
			}
	break;
/* #line 517 "user/ini_parser.c" */
		}
	}
	}

	_out: {}
	}

/* #line 283 "user/ini_parser.rl" */

}
