//
// Created by MightyPork on 2017/08/20.
//
// Short sequences: (* = not implemented)
//
// ESC char
//   ESC 6     Back Index (DECBI), VT420 and up.
//   ESC 7     Save Cursor (DECSC).
//   ESC 8     Restore Cursor (DECRC).
//   ESC 9     Forward Index (DECFI), VT420 and up.
//   ESC =     Application Keypad (DECKPAM).
//   ESC >     Normal Keypad (DECKPNM).
//   ESC F     Cursor to lower left corner of screen.
//   ESC c     Full Reset (RIS).
//  *ESC l     Memory Lock (per HP terminals). Locks memory above the cursor.
//  *ESC m     Memory Unlock (per HP terminals).
//   ESC n     Invoke the G2 Character Set as GL (LS2).
//   ESC o     Invoke the G3 Character Set as GL (LS3).
//  *ESC |     Invoke the G3 Character Set as GR (LS3R).
//  *ESC }     Invoke the G2 Character Set as GR (LS2R).
//  *ESC ~     Invoke the G1 Character Set as GR (LS1R).
//
// ESC # Ps
//  *ESC # 3   DEC double-height line, top half (DECDHL).
//  *ESC # 4   DEC double-height line, bottom half (DECDHL).
//  *ESC # 5   DEC single-width line (DECSWL).
//  *ESC # 6   DEC double-width line (DECDWL).
//   ESC # 8   DEC Screen Alignment Test (DECALN).
//
// ESC SP char
//  *ESC SP F  7-bit controls (S7C1T).
//  *ESC SP G  8-bit controls (S8C1T).
//  *ESC SP L  Set ANSI conformance level 1 (dpANS X3.134.1).
//  *ESC SP M  Set ANSI conformance level 2 (dpANS X3.134.1).
//  *ESC SP N  Set ANSI conformance level 3 (dpANS X3.134.1).
//
// Charset commands
//   ESC ( char  Designate G0 character set ('0' = symbols, 'B' = ASCII-US, 'A' = ASCII-UK)
//   ESC ) char  Designate G1 character set
//   SO          Switch to G0 character set
//   SI          Switch to G1 character set
//

#include <esp8266.h>
#include <httpclient.h>
#include "apars_short.h"
#include "apars_logging.h"
#include "screen.h"

// ----- Character Set ---

/**
 * Command to assign G0 or G1
 * @param slot - ( or ) for G0 or G1
 * @param c - character table ID (0, B etc)
 */
void ICACHE_FLASH_ATTR
apars_handle_chs_designate(char slot, char c)
{
	if (slot == '(') screen_set_charset(0, c); // G0
	else if (slot == ')') screen_set_charset(1, c); // G1
	else {
		ansi_noimpl("ESC %c %c", slot, c);
	}
	// other alternatives * + . - / not implemented
}

/** Select charset slot */
void ICACHE_FLASH_ATTR
apars_handle_chs_switch(int Gx)
{
	screen_set_charset_n(Gx);
}

// ----- ESC SP char -----

/**
 * ESC SP <c> (this sets 8/7-bit mode and some other archaic options)
 * @param c - key character
 */
void ICACHE_FLASH_ATTR
apars_handle_space_cmd(char c)
{
	ansi_noimpl("ESC SP %c", c);
}

// ----- ESC # num -----

/**
 * Codes in the format ESC # n
 * @param c - the trailing symbol (numeric ASCII)
 */
void ICACHE_FLASH_ATTR apars_handle_hash_cmd(char c)
{
	switch(c) {
		case '3': // Double size, top half
		case '4': // Single size, bottom half
		case '5': // Single width, single height
		case '6': // Double width
			ansi_noimpl("Double Size Line");
			break;

		case '8':
			screen_fill_with_E();
			break;

		// development codes - do not use!
		case '7':
			http_get("http://example.com", NULL);
			break;

		default:
			ansi_noimpl("ESC # %c", c);
	}
}

// ----- ESC char -----

/**
 * Single-character escape codes (ESC x)
 * @param c - the trailing symbol (ASCII)
 */
void ICACHE_FLASH_ATTR apars_handle_short_cmd(char c)
{
	switch(c) {
		case 'c': // screen reset
			screen_reset();
			break;

		case '6': // back index
			screen_back_index(1);
			break;

		case '7': // save cursor + attributes
			screen_cursor_save(true);
			break;

		case '8': // restore cursor + attributes
			screen_cursor_restore(true);
			break;

		case 'E': // same as CR LF
			screen_cursor_move(1, 0, false);
			screen_cursor_set_x(0);
			break;

		case 'F': // bottom left
			screen_cursor_set(termconf_live.height-1, 0);
			break;

		case 'D': // move cursor down, scroll screen up if needed
			screen_cursor_move(1, 0, true);
			break;

		case 'M': // move cursor up, scroll screen down if needed
			screen_cursor_move(-1, 0, true);
			break;

		case 'H':
			screen_set_tab();
			break;

		case '>':
			screen_set_numpad_alt_mode(false);
			break;

		case '<': // "Enter ANSI / VT100 mode" - no-op (we don't support VT52 mode)
			break;

		case '=':
			screen_set_numpad_alt_mode(true);
			break;

		case '|': // Invoke the G3 Character Set as GR (LS3R).
		case '}': // Invoke the G2 Character Set as GR (LS2R).
		case '~': // Invoke the G1 Character Set as GR (LS1R).
			// Those do not seem to do anything TODO investigate
			break;

		case '@': // no-op padding char (?)
			break;

		case '\\': // spurious string terminator
			break;

		default:
			ansi_noimpl("ESC %c", c);
	}
}
