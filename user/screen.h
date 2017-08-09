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
 * ints are 0-based, left-top is the origin.
 * X grows to the right, Y to the bottom.
 *
 * +---->
 * |    X
 * |
 * V Y
 *
 */

// Size designed for the wifi config structure
// Must be constant to avoid corrupting user config after upgrade
#define TERMCONF_SIZE 200

#define TERM_BTN_LEN 10
#define TERM_TITLE_LEN 64

typedef enum {
	CHANGE_CONTENT,
	CHANGE_LABELS,
} ScreenNotifyChangeTopic;

#define SCREEN_NOTIFY_DELAY_MS 20

typedef struct {
	u32 width;
	u32 height;
	u8 default_bg;
	u8 default_fg;
	char title[TERM_TITLE_LEN];
	char btn[5][TERM_BTN_LEN];
	u8 theme;
	u32 parser_tout_ms;
} TerminalConfigBundle;

// Live config
extern TerminalConfigBundle * const termconf;

/**
 * Transient live config with no persist, can be modified via esc sequences.
 * terminal_apply_settings() copies termconf to this struct, erasing old scratch changes
 */
extern TerminalConfigBundle termconf_scratch;

void terminal_restore_defaults(void);
void terminal_apply_settings(void);
void terminal_apply_settings_noclear(void); // the same, but with no screen reset / init

/**
 * Maximum screen size (determines size of the static data array)
 *
 * TODO May need adjusting if there are size problems when flashing the ESP.
 * We could also try to pack the Cell struct to a single 32bit word.
 */
#define MAX_SCREEN_SIZE (80*30)

typedef enum {
	CLEAR_TO_CURSOR=0, CLEAR_FROM_CURSOR=1, CLEAR_ALL=2
} ClearMode;

typedef uint8_t Color;

httpd_cgi_state screenSerializeToBuffer(char *buffer, size_t buf_len, void **data);

void screenSerializeLabelsToBuffer(char *buffer, size_t buf_len);

typedef struct {
	u8 lsb;
	u8 msb;
} WordB2;

/** Encode number to two nice ASCII bytes */
void encode2B(u16 number, WordB2 *stru);

/** Init the screen */
void screen_init(void);
/** Change the screen size */
void screen_resize(int rows, int cols);
/** Check if coord is valid */
bool screen_isCoordValid(int y, int x);

// --- Clearing ---

/** Screen reset to default state */
void screen_reset(void);
/** Clear entire screen */
void screen_clear(ClearMode mode);
/** Clear line */
void screen_clear_line(ClearMode mode);
/** Clear part of line */
void screen_clear_in_line(unsigned int count);
/** Shift screen upwards */
void screen_scroll_up(unsigned int lines);
/** Shift screen downwards */
void screen_scroll_down(unsigned int lines);
/** esc # 8 - fill entire screen with E of default colors */
void screen_fill_with_E(void);

// --- insert / delete ---
void screen_insert_lines(unsigned int lines);
void screen_delete_lines(unsigned int lines);
void screen_insert_characters(unsigned int count);
void screen_delete_characters(unsigned int count);

// --- Cursor control ---

/** Set cursor position */
void screen_cursor_set(int y, int x);
/** Read cursor pos to given vars */
void screen_cursor_get(int *y, int *x);
/** Set cursor X position */
void screen_cursor_set_x(int x);
/** Set cursor Y position */
void screen_cursor_set_y(int y);
/** Reset cursor attribs */
void screen_reset_cursor(void);
/** Relative cursor move */
void screen_cursor_move(int dy, int dx, bool scroll);
/** Save the cursor pos */
void screen_cursor_save(bool withAttrs);
/** Restore the cursor pos */
void screen_cursor_restore(bool withAttrs);
/** Enable cursor display */
void screen_cursor_enable(bool enable);
/** Enable auto wrap */
void screen_wrap_enable(bool enable);

// --- Colors ---

/** Set cursor foreground color */
void screen_set_fg(Color color);
/** Set cursor background coloor */
void screen_set_bg(Color color);
/** make foreground bright */
void screen_set_bold(bool bold);
/** Set cursor foreground and background color */
void screen_set_colors(Color fg, Color bg);
/** Invert colors */
void screen_inverse(bool inverse);

/**
 * Set a character in the cursor color, move to right with wrap.
 * The character may be ASCII (then only one char is used), or
 * unicode (then it can be 4 chars, or terminated by a zero)
 */
void screen_putchar(const char *ch);

#if 0
/** Debug dump */
void screen_dd(void);
#endif

extern void screen_notifyChange(ScreenNotifyChangeTopic topic);

#endif // SCREEN_H
