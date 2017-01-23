#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>
#include <stdbool.h>
#include <esp8266.h>
#include <httpd.h>

/**
 * This module handles the virtual screen and operations on it.
 *
 * It is interfaced by calls from the ANSI parser, and the screen
 * data can be rendered for the front-end.
 *
 * ---
 *
 * Colors are 0-15, 0-7 dim, 8-15 bright.
 *
 * NORMAL
 * 0 black, 1 red, 2 green, 3 yellow
 * 4 blue, 5 mag, 6 cyan, 7 white
 *
 * BRIGHT
 * 8 black, 9 red, 10 green, 11 yellow
 * 12 blue, 13 mag, 14 cyan, 15 white
 *
 * Coordinates are 0-based, left-top is the origin.
 * X grows to the right, Y to the bottom.
 *
 * +---->
 * |    X
 * |
 * V Y
 *
 */

/**
 * Maximum screen size (determines size of the static data array)
 *
 * TODO May need adjusting if there are size problems when flashing the ESP.
 * We could also try to pack the Cell struct to a single 32bit word.
 */
#define MAX_SCREEN_SIZE (80*25)

#define SCREEN_DEF_W 26  //!< Default screen width
#define SCREEN_DEF_H 10  //!< Default screen height

#define SCREEN_DEF_BG 0  //!< Default screen background
#define SCREEN_DEF_FG 7  //!< Default screen foreground

typedef enum {
	CLEAR_TO_CURSOR=0, CLEAR_FROM_CURSOR=1, CLEAR_ALL=2
} ClearMode;

typedef uint8_t Color;
typedef unsigned int Coordinate;

httpd_cgi_state ICACHE_FLASH_ATTR
screenSerializeToBuffer(char *buffer, size_t buf_len, void **data);

/** Init the screen */
void screen_init(void);

/** Change the screen size */
void screen_resize(Coordinate w, Coordinate h);

// --- Clearing ---

/** Screen reset to default state */
void screen_reset(void);

/** Clear entire screen, set all to 7 on 0 */
void screen_clear(ClearMode mode);

/** Line reset to gray-on-white, empty */
void screen_clear_line(ClearMode mode);

/** Shift screen upwards */
void screen_scroll_up(unsigned int lines);

/** Shift screen downwards */
void screen_scroll_down(unsigned int lines);

// --- Cursor control ---

/** Set cursor position */
void screen_cursor_set(Coordinate x, Coordinate y);

/** Set cursor X position */
void screen_cursor_set_x(Coordinate x);

/** Set cursor Y position */
void screen_cursor_set_y(Coordinate y);

/** Relative cursor move */
void screen_cursor_move(int dx, int dy);

/** Save the cursor pos */
void screen_cursor_save(void);

/** Restore the cursor pos */
void screen_cursor_restore(void);

/** Enable cursor display */
void screen_cursor_enable(bool enable);

// --- Colors ---

/** Set cursor foreground color */
void screen_set_fg(Color color);

/** Set cursor background coloor */
void screen_set_bg(Color color);

/** make foreground bright */
void screen_set_bright_fg(void);

/** Set cursor foreground and background color */
void screen_set_colors(Color fg, Color bg);

/** Invert colors */
void screen_inverse(bool inverse);


/** Set a character in the cursor color, move to right with wrap. */
void screen_putchar(char c);

#if 0
/** Debug dump */
void screen_dd(void);
#endif

extern void screen_notifyChange(void);

#endif // SCREEN_H
