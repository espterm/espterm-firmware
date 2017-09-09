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

// Size designed for the terminal config structure
// Must be constant to avoid corrupting user config after upgrade
#define TERMCONF_SIZE 300

#define TERM_BTN_LEN 10
#define TERM_BTN_MSG_LEN 10
#define TERM_TITLE_LEN 64
#define TERM_BTN_COUNT 5

#define SCR_DEF_DISPLAY_TOUT_MS 10
#define SCR_DEF_DISPLAY_COOLDOWN_MS 30
#define SCR_DEF_PARSER_TOUT_MS 10
#define SCR_DEF_FN_ALT_MODE true // true - SS3 codes, easier to parse & for xterm compatibility
#define SCR_DEF_WIDTH 26
#define SCR_DEF_HEIGHT 10
#define SCR_DEF_TITLE "ESPTerm"

/** Maximum screen size (determines size of the static data array) */
#define MAX_SCREEN_SIZE (80*25)

#define TERMCONF_VERSION 4

// --- Persistent Settings ---

enum CursorShape {
	CURSOR_BLOCK_BL = 0,
	CURSOR_DEFAULT  = 1, // this is translated to a user configured style
	CURSOR_BLOCK = 2,
	CURSOR_UNDERLINE_BL = 3,
	CURSOR_UNDERLINE = 4,
	CURSOR_BAR_BL = 5,
	CURSOR_BAR = 6,
};

typedef struct {
	u32 width;
	u32 height;
	u8 default_bg; // should be the Color typedef, but this way the size is more explicit
	u8 default_fg;
	char title[TERM_TITLE_LEN];
	char btn[TERM_BTN_COUNT][TERM_BTN_LEN];
	u8 theme;
	u32 parser_tout_ms;
	u32 display_tout_ms;
	bool fn_alt_mode; // xterm compatibility mode (alternate codes for some FN keys)
	u8 config_version;
	u32 display_cooldown_ms;
	bool loopback;
	bool show_buttons;
	bool show_config_links;
	char btn_msg[TERM_BTN_COUNT][TERM_BTN_MSG_LEN];
	enum CursorShape def_cursor_shape;
} TerminalConfigBundle;

// Live config
extern TerminalConfigBundle * const termconf;

/**
 * Transient live config with no persist, can be modified via esc sequences.
 * terminal_apply_settings() copies termconf to this struct, erasing old scratch changes
 */
extern TerminalConfigBundle termconf_scratch;

enum MTM {
	MTM_NONE = 0,
	MTM_X10 = 1,
	MTM_NORMAL = 2,
	MTM_BUTTON_MOTION = 3, // any higher mode tracks motion
	MTM_ANY_MOTION = 4,
};

enum MTE {
	MTE_SIMPLE = 0,
	MTE_UTF8 = 1,
	MTE_SGR = 2,
	MTE_URXVT = 3,
};

typedef struct {
	enum MTM mode;
	bool focus_tracking;
	enum MTE encoding;
} MouseTrackingConfig;

extern MouseTrackingConfig mouse_tracking;

/** Restore default settings to termconf. Does not apply or copy to scratch. */
void terminal_restore_defaults(void);
/** Apply settings, redraw (clears the screen) */
void terminal_apply_settings(void);
/** Apply settings, redraw (no clear - not resized) */
void terminal_apply_settings_noclear(void);
/** Init the screen */
void screen_init(void);
/** Change the screen size */
void screen_resize(int rows, int cols);
/** Set screen title */
void screen_set_title(const char *title);
/** Set a button text */
void screen_set_button_text(int num, const char *text);

// --- Encoding ---

typedef enum {
	CS_B_USASCII = 'B',
	CS_A_UKASCII = 'A',
	CS_0_DEC_SUPPLEMENTAL = '0',
	CS_1_DOS_437 = '1',
} CHARSET;

httpd_cgi_state screenSerializeToBuffer(char *buffer, size_t buf_len, void **data);

void screenSerializeLabelsToBuffer(char *buffer, size_t buf_len);

// --- Clearing ---

typedef enum {
	CLEAR_TO_CURSOR = 0,
	CLEAR_FROM_CURSOR = 1,
	CLEAR_ALL = 2
} ClearMode;

/** Screen reset to default state */
void screen_reset(void);
/** Clear entire screen */
void screen_clear(ClearMode mode);
/** Clear line */
void screen_clear_line(ClearMode mode);
/** Clear part of line */
void screen_clear_in_line(unsigned int count);
/** Swap to alternate buffer (really what this does is backup terminal title, size and other global attributes) */
void screen_swap_state(bool alternate);

// --- insert / delete ---

/** Insert lines at cursor, shove down */
void screen_insert_lines(unsigned int lines);
/** Delete lines at cursor, pull up */
void screen_delete_lines(unsigned int lines);
/** Insert characters at cursor, shove right */
void screen_insert_characters(unsigned int count);
/** Delete characters at cursor, pull left */
void screen_delete_characters(unsigned int count);
/** Shift screen upwards */
void screen_scroll_up(unsigned int lines);
/** Shift screen downwards */
void screen_scroll_down(unsigned int lines);

// --- Cursor control ---

/** Set cursor shape */
void screen_cursor_shape(enum CursorShape shape);
/** Toggle cursor blink (modifies shape) */
void screen_cursor_blink(bool blink);
/** Set cursor position */
void screen_cursor_set(int y, int x);
/** Read cursor pos to given vars */
void screen_cursor_get(int *y, int *x);
/** Set cursor X position */
void screen_cursor_set_x(int x);
/** Set cursor Y position */
void screen_cursor_set_y(int y);
/** Relative cursor move */
void screen_cursor_move(int dy, int dx, bool scroll);
/** Save the cursor pos */
void screen_cursor_save(bool withAttrs);
/** Restore the cursor pos */
void screen_cursor_restore(bool withAttrs);

// --- Cursor behavior setting ---

/** Toggle INSERT / REPLACE */
void screen_set_insert_mode(bool insert);
/** Enable CR auto */
void screen_set_newline_mode(bool nlm);
/** Enable auto wrap */
void screen_wrap_enable(bool enable);
/** Enable reverse wrap-around */
void screen_reverse_wrap_enable(bool enable);
/** Set scrolling region */
void screen_set_scrolling_region(int from, int to);
/** Enable or disable origin remap to top left of scrolling region */
void screen_set_origin_mode(bool region_origin);

// --- Graphic rendition setting ---

typedef uint8_t Color; // 0-16

#define ATTR_BOLD (1<<0)
#define ATTR_FAINT (1<<1)
#define ATTR_ITALIC (1<<2)
#define ATTR_UNDERLINE (1<<3)
#define ATTR_BLINK (1<<4)
#define ATTR_FRAKTUR (1<<5)
#define ATTR_STRIKE (1<<6)

/** Set cursor foreground color */
void screen_set_fg(Color color);
/** Set cursor background coloor */
void screen_set_bg(Color color);
/** Enable/disable attrs by bitmask */
void screen_set_sgr(u8 attrs, bool ena);
/** Set the inverse attribute */
void screen_set_sgr_inverse(bool ena);
/** Reset cursor attribs */
void screen_reset_sgr(void);

// --- Global modes and attributes ---

/** Enable cursor display */
void screen_set_cursor_visible(bool visible);
/** Toggle application keypad mode */
void screen_set_numpad_alt_mode(bool app_mode);
/** Toggle application cursor mode */
void screen_set_cursors_alt_mode(bool app_mode);
/** Set reverse video mode */
void screen_set_reverse_video(bool reverse);

// --- Charset ---

/** Switch G0 <-> G1 */
void screen_set_charset_n(int Gx);
/** Assign G0 or G1 */
void screen_set_charset(int Gx, char charset);

// --- Tab stops ---

/** Remove all tabs on the screen */
void screen_clear_all_tabs(void);
/** Set tab at the current column */
void screen_set_tab(void);
/** Remove tab at the current column (if any) */
void screen_clear_tab(void);
/** Move forward one tab */
void screen_tab_forward(int count);
/** Move backward one tab */
void screen_tab_reverse(int count);
/** Move left, shift right if at the boundary */
void screen_back_index(int count);

// --- Printing characters ---

/**
 * Set a character in the cursor color, move to right with wrap.
 * The character may be ASCII (then only one char is used), or
 * unicode (then it can be 4 chars, or terminated by a zero)
 */
void screen_putchar(const char *ch);
/**
 * esc # 8 - fill entire screen with E of default colors
 * (DEC alignment test mode)
 */
void screen_fill_with_E(void);

// --- Queries ---

/** Report current SGR as num;num;... for DAC query */
void screen_report_sgr(char *buffer);

// --- Notify ---

typedef enum {
	CHANGE_CONTENT = 0,
	CHANGE_LABELS = 1,
} ScreenNotifyChangeTopic;

/**
 * Called when the screen content or settings change
 * and the front-end should redraw / update.
 * @param topic - what kind of change this is (chooses what message to send)
 */
extern void screen_notifyChange(ScreenNotifyChangeTopic topic);

#endif // SCREEN_H
