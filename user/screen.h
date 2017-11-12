#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>
#include <stdbool.h>
#include <esp8266.h>
#include <httpd.h>
#include "config_xmacros.h"

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

#define TERM_BTN_LEN 10
#define TERM_BTN_MSG_LEN 10
#define TERM_TITLE_LEN 64
#define TERM_BTN_COUNT 5
#define TERM_BACKDROP_LEN 100
#define TERM_FONTSTACK_LEN 100

#define SCR_DEF_DISPLAY_TOUT_MS 12
#define SCR_DEF_DISPLAY_COOLDOWN_MS 35
#define SCR_DEF_PARSER_TOUT_MS 0
#define SCR_DEF_FN_ALT_MODE true // true - SS3 codes, easier to parse & for xterm compatibility
#define SCR_DEF_WIDTH 26
#define SCR_DEF_HEIGHT 10
#define SCR_DEF_TITLE "ESPTerm"

/** Maximum screen size (determines size of the static data array) */
#define MAX_SCREEN_SIZE (80*25)

enum CursorShape {
	CURSOR_BLOCK_BL = 0,
	CURSOR_DEFAULT  = 1, // this is translated to a user configured style
	CURSOR_BLOCK = 2,
	CURSOR_UNDERLINE_BL = 3,
	CURSOR_UNDERLINE = 4,
	CURSOR_BAR_BL = 5,
	CURSOR_BAR = 6,
};

#define SCR_DEF_SHOW_BUTTONS 1
#define SCR_DEF_SHOW_MENU 1
#define SCR_DEF_CURSOR_SHAPE CURSOR_BLOCK_BL
#define SCR_DEF_CRLF 0  // enter sends CRLF
#define SCR_DEF_ALLFN 0 // capture F5 etc
#define SCR_DEF_DEBUGBAR 0
#define SCR_DEF_DECOPT12 0
#define SCR_DEF_ASCIIDEBUG 0
#define SCR_DEF_BUTTON_COUNT 5

// --- Persistent Settings ---
#define CURSOR_BLINKS(shape) ((shape)==CURSOR_BLOCK_BL||(shape)==CURSOR_UNDERLINE_BL||(shape)==CURSOR_BAR_BL)

// Size designed for the terminal config structure
// Must be constant to avoid corrupting user config after upgrade
#define TERMCONF_SIZE 500
#define TERMCONF_VERSION 6

//....Type................Name..Suffix...............Deref..XGET.........Cast..XSET...........NOTIFY..Allow
// Deref is used to pass the field to xget. Cast is used to convert the &'d field to what xset wants (needed for static arrays)
#define XTABLE_TERMCONF \
	X(u32,            width, /**/,                  /**/, xget_dec,      xset_u32, NULL,      /**/, 1) \
	X(u32,            height, /**/,                 /**/, xget_dec,      xset_u32, NULL,      /**/, 1) \
	X(u32,            default_bg, /**/,             /**/, xget_term_color,  xset_term_color, NULL,      /**/, 1) \
	X(u32,            default_fg, /**/,             /**/, xget_term_color,  xset_term_color, NULL,      /**/, 1) \
	X(char,           title, [TERM_TITLE_LEN],      /**/, xget_string,   xset_string, TERM_TITLE_LEN, /**/, 1) \
	X(char,           btn1, [TERM_BTN_LEN],         /**/, xget_string,   xset_string, TERM_BTN_LEN,   /**/, 1) \
	X(char,           btn2, [TERM_BTN_LEN],         /**/, xget_string,   xset_string, TERM_BTN_LEN,   /**/, 1) \
	X(char,           btn3, [TERM_BTN_LEN],         /**/, xget_string,   xset_string, TERM_BTN_LEN,   /**/, 1) \
	X(char,           btn4, [TERM_BTN_LEN],         /**/, xget_string,   xset_string, TERM_BTN_LEN,   /**/, 1) \
	X(char,           btn5, [TERM_BTN_LEN],         /**/, xget_string,   xset_string, TERM_BTN_LEN,   /**/, 1) \
	X(u8,             theme, /**/,                  /**/, xget_dec,      xset_u8, NULL,       /**/, 1) \
	X(u32,            parser_tout_ms, /**/,         /**/, xget_dec,      xset_u32, NULL,      /**/, 1) \
	X(u32,            display_tout_ms, /**/,        /**/, xget_dec,      xset_u32, NULL,      /**/, 1) \
	X(bool,           fn_alt_mode, /**/,            /**/, xget_bool,     xset_bool, NULL,     /**/, 1) \
	X(u8,             config_version, /**/,         /**/, xget_dec,      xset_u8, NULL,       /**/, 1) \
	X(u32,            display_cooldown_ms, /**/,    /**/, xget_dec,      xset_u32, NULL,      /**/, 1) \
	X(bool,           loopback, /**/,               /**/, xget_bool,     xset_bool, NULL,     /**/, 1) \
	X(bool,           show_buttons, /**/,           /**/, xget_bool,     xset_bool, NULL,     /**/, 1) \
	X(bool,           show_config_links, /**/,      /**/, xget_bool,     xset_bool, NULL,     /**/, 1) \
	X(char,           bm1, [TERM_BTN_MSG_LEN],      /**/, xget_term_bm,  xset_term_bm, NULL,  /**/, 1) \
	X(char,           bm2, [TERM_BTN_MSG_LEN],      /**/, xget_term_bm,  xset_term_bm, NULL,  /**/, 1) \
	X(char,           bm3, [TERM_BTN_MSG_LEN],      /**/, xget_term_bm,  xset_term_bm, NULL,  /**/, 1) \
	X(char,           bm4, [TERM_BTN_MSG_LEN],      /**/, xget_term_bm,  xset_term_bm, NULL,  /**/, 1) \
	X(char,           bm5, [TERM_BTN_MSG_LEN],      /**/, xget_term_bm,  xset_term_bm, NULL,  /**/, 1) \
	X(u32,            cursor_shape, /**/,           /**/, xget_dec,      xset_term_cursorshape, NULL,      /**/, 1) \
	X(bool,           crlf_mode, /**/,              /**/, xget_bool,     xset_bool, NULL,     /**/, 1) \
	X(bool,           want_all_fn, /**/,            /**/, xget_bool,     xset_bool, NULL,     /**/, 1) \
	X(bool,           debugbar, /**/,               /**/, xget_bool,     xset_bool, NULL,     /**/, 1) \
	X(bool,           allow_decopt_12, /**/,        /**/, xget_bool,     xset_bool, NULL,     /**/, 1) \
	X(bool,           ascii_debug, /**/,            /**/, xget_bool,     xset_bool, NULL,     /**/, 1) \
	X(char,           backdrop, [TERM_BACKDROP_LEN], /**/, xget_string,  xset_string, TERM_BACKDROP_LEN,  /**/, 1) \
	X(u8,             button_count, /**/,            /**/, xget_dec,     xset_u8, NULL,       /**/, 1) \
	X(u32,            bc1, /**/,      /**/, xget_term_color,  xset_term_color, NULL,  /**/, 1) \
	X(u32,            bc2, /**/,      /**/, xget_term_color,  xset_term_color, NULL,  /**/, 1) \
	X(u32,            bc3, /**/,      /**/, xget_term_color,  xset_term_color, NULL,  /**/, 1) \
	X(u32,            bc4, /**/,      /**/, xget_term_color,  xset_term_color, NULL,  /**/, 1) \
	X(u32,            bc5, /**/,      /**/, xget_term_color,  xset_term_color, NULL,  /**/, 1) \
	X(char,           font_stack, [TERM_FONTSTACK_LEN], /**/, xget_string,  xset_string, TERM_FONTSTACK_LEN,  /**/, 1) \
	X(u8,             font_size, /**/,              /**/, xget_dec,      xset_u8, NULL,       /**/, 1)

#define TERM_BM_N(tc, n) ((tc)->bm1+(TERM_BTN_MSG_LEN*n))
#define TERM_BTN_N(tc, n) ((tc)->btn1+(TERM_BTN_LEN*n))

/** Export color for config */
void xget_term_color(char *buff, u32 value);
/** Export button message as stirng for config */
void xget_term_bm(char *buff, char *value);
/** Set button message */
enum xset_result xset_term_bm(const char *name, char *field, const char *buff, const void *arg);
/** Set color */
enum xset_result xset_term_color(const char *name, u32 *field, const char *buff, const void *arg);
/** Set cursor shape */
enum xset_result xset_term_cursorshape(const char *name, u32 *field, const char *buff, const void *arg);

typedef struct {
#define X XSTRUCT_FIELD
	XTABLE_TERMCONF
#undef X
} TerminalConfigBundle;

// Live config
extern TerminalConfigBundle * const termconf;

/**
 * Transient live config with no persist, can be modified via esc sequences.
 * terminal_apply_settings() copies termconf to this struct, erasing old scratch changes
 */
extern TerminalConfigBundle termconf_live;

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
void screen_set_button_message(int num, const char *msg);
void screen_set_button_color(int num, const char *buf);
void screen_set_button_count(int count);
/** Change backdrop */
void screen_set_backdrop(const char *url);

// --- Encoding ---

typedef enum {
	CS_B_USASCII = 'B',
	CS_A_UKASCII = 'A',
	CS_0_DEC_SUPPLEMENTAL = '0',
	CS_1_DOS_437 = '1',
	CS_2_BLOCKS_LINES = '2',
	CS_3_LINES_EXTRA = '3',
} CHARSET;

enum ScreenSerializeTopic {
	TOPIC_CHANGE_SCREEN_OPTS  = (1<<0),
	TOPIC_CHANGE_CONTENT_ALL  = (1<<1),
	TOPIC_CHANGE_CONTENT_PART = (1<<2),
	TOPIC_CHANGE_TITLE        = (1<<3),
	TOPIC_CHANGE_BUTTONS      = (1<<4),
	TOPIC_CHANGE_CURSOR       = (1<<5),
	TOPIC_INTERNAL            = (1<<6), // debugging internal state
	TOPIC_BELL                = (1<<7), // beep
	TOPIC_CHANGE_BACKDROP     = (1<<8),
	TOPIC_CHANGE_STATIC_OPTS  = (1<<9),
	TOPIC_DOUBLE_LINES        = (1<<10),
	TOPIC_FLAG_NOCLEAN        = (1<<15), // do not clean dirty extents

	// combos
	TOPIC_INITIAL =
		TOPIC_CHANGE_SCREEN_OPTS |
		TOPIC_CHANGE_STATIC_OPTS |
		TOPIC_CHANGE_CONTENT_ALL |
		TOPIC_CHANGE_CURSOR |
		TOPIC_CHANGE_TITLE |
		TOPIC_CHANGE_BACKDROP |
		TOPIC_CHANGE_BUTTONS |
		TOPIC_DOUBLE_LINES,
};

typedef u16 ScreenNotifyTopics;

httpd_cgi_state screenSerializeToBuffer(char *buffer, size_t buf_len, ScreenNotifyTopics topics, void **data);

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
/* Report scrolling region */
void screen_region_get(int *pv0, int *pv1);
/** Enable or disable origin remap to top left of scrolling region */
void screen_set_origin_mode(bool region_origin);

// --- Graphic rendition setting ---

typedef uint8_t Color;
typedef uint16_t CellAttrs;

enum SgrAttrBits {
	ATTR_FG        = (1<<0),  //!< 1 if not using default background color (ignore cell bg) - color extension bit
	ATTR_BG        = (1<<1),  //!< 1 if not using default foreground color (ignore cell fg) - color extension bit
	ATTR_BOLD      = (1<<2),  //!< Bold font
	ATTR_UNDERLINE = (1<<3),  //!< Underline decoration
	ATTR_INVERSE   = (1<<4),  //!< Invert colors - this is useful so we can clear then with SGR manipulation commands
	ATTR_BLINK     = (1<<5),  //!< Blinking
	ATTR_ITALIC    = (1<<6),  //!< Italic font
	ATTR_STRIKE    = (1<<7),  //!< Strike-through decoration
	ATTR_OVERLINE  = (1<<8),  //!< Over-line decoration
	ATTR_FAINT     = (1<<9),  //!< Faint foreground color (reduced alpha)
	ATTR_FRAKTUR   = (1<<10), //!< Fraktur font (unicode substitution)
};

/** Set cursor foreground color */
void screen_set_fg(Color color);
/** Set cursor background coloor */
void screen_set_bg(Color color);
/** Set cursor foreground color to default */
void screen_set_default_fg(void);
/** Set cursor background color to default */
void screen_set_default_bg(void);
/** Enable/disable attrs by bitmask */
void screen_set_sgr(CellAttrs attrs, bool ena);
/** Conceal style */
void screen_set_sgr_conceal(bool ena);
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
/** Save DECOPT */
void screen_save_private_opt(int n);
/** Restore DECOPT */
void screen_restore_private_opt(int n);
/** Set bracketed paste */
void screen_set_bracketed_paste(bool ena);

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

/** Set line attribs; 0-no change, 1,2 - single,double */
void screen_set_line_attr(uint8_t double_w, uint8_t double_h_top, uint8_t double_h_bot);

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

/**
 * Repeat last graphic character
 * @param count
 */
void screen_repeat_last_character(int count);

// --- Queries ---

/** Report current SGR as num;num;... for DAC query */
void screen_report_sgr(char *buffer);

/**
 * Called when the screen content or settings change
 * and the front-end should redraw / update.
 * @param topic - what kind of change this is (chooses what message to send)
 */
extern void screen_notifyChange(ScreenNotifyTopics topics);

#define seri_dbg(...)
#define seri_warn(...) warn(__VA_ARGS__)

#endif // SCREEN_H
