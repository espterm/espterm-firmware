#include <esp8266.h>
#include <httpd.h>
#include "screen.h"
#include "persist.h"
#include "sgr.h"
#include "ascii.h"
#include "apars_logging.h"
#include "jstring.h"
#include "character_sets.h"
#include "utf8.h"

TerminalConfigBundle * const termconf = &persist.current.termconf;
TerminalConfigBundle termconf_live;

MouseTrackingConfig mouse_tracking;

// forward declare
static void utf8_remap(char* out, char g, char charset);

#define W termconf_live.width
#define H termconf_live.height

/**
 * Highest permissible value of the color attribute
 */
#define COLOR_MAX 15

/**
 * Screen cell data type (16 bits)
 */
typedef struct __attribute__((packed)){
	UnicodeCacheRef symbol : 8;
	Color fg : 8;
	Color bg : 8;
	u8 attrs;
} Cell;

/**
 * The screen data array
 */
static Cell screen[MAX_SCREEN_SIZE];



#define TABSTOP_WORDS 5
/**
 * Screen state structure
 */
static struct {
	bool numpad_alt_mode;  //!< DECNKM - Numpad Application Mode
	bool cursors_alt_mode; //!< DECCKM - Cursors Application mode
	bool bracketed_paste;

	bool reverse_video;            //!< DECSCNM - Reverse video

	bool insert_mode;    //!< IRM - Insert mode (move rest of the line to the right)
	bool cursor_visible; //!< DECTCEM - Cursor visible

	// Vertical margin bounds (inclusive start/end of scrolling region)
	int vm0;
	int vm1;

	u32 tab_stops[TABSTOP_WORDS]; // tab stops bitmap
	char last_char[4];
} scr;

#define R0 scr.vm0
#define R1 scr.vm1
#define RH (scr.vm1 - scr.vm0 + 1)
// horizontal edges - will be useful if horizontal margin is implemented
#define C0 0
#define C1 (W-1)

typedef struct {
	/* Cursor position */
	int x;        //!< X coordinate
	int y;        //!< Y coordinate
	bool hanging; //!< xenl state - cursor half-wrapped

	/* SGR */
	bool inverse; //!< not in attrs bc it's applied server-side (not sent to browser)
	bool conceal;  //!< similar to inverse, causes all to be replaced by SP
	u8 attrs;
	Color fg;     //!< Foreground color for writing
	Color bg;     //!< Background color for writing

	// Other attribs

	/* Character set */
	int charsetN;
	char charset0;
	char charset1;

	/** Options saved with cursor */
	bool auto_wrap;      //!< DECAWM - Wrapping when EOL
	bool reverse_wrap; //!< Reverse-wraparound Mode. DECSET 45
	bool origin_mode;    //!< DECOM - absolute positioning is relative to vertical margins
} CursorTypeDef;

/**
 * Cursor position and attributes
 */
static CursorTypeDef cursor;

/**
 * Saved cursor position, used with the SCP RCP commands
 */
static CursorTypeDef cursor_sav;
bool cursor_saved = false;

/** This structure holds old state when switching to an alternate buffer */
static struct {
	bool alternate_active;
	char title[TERM_TITLE_LEN];
	char btn[TERM_BTN_COUNT][TERM_BTN_LEN];
	char btn_msg[TERM_BTN_COUNT][TERM_BTN_MSG_LEN];
	u32 width;
	u32 height;
	int vm0;
	int vm1;
	u32 tab_stops[TABSTOP_WORDS];
} state_backup;

/** options backup (save/restore) */
static struct {
	bool cursors_alt_mode;
	bool reverse_video;
	bool origin_mode;
	bool auto_wrap;
	enum MTM mouse_tracking;
	enum MTE mouse_encoding;
	bool focus_tracking;
	bool cursor_blink;
	bool cursor_visible;
	bool reverse_wrap;
	bool bracketed_paste;
	bool show_buttons;
	bool show_config_links;
} opt_backup;

/**
 * This is used to prevent premature change notifications
 * (from nested calls)
 */
static volatile int notifyLock = 0;

#define NOTIFY_LOCK()   do { \
							notifyLock++; \
						} while(0)

#define NOTIFY_DONE()   do { \
							if (notifyLock > 0) notifyLock--; \
							if (notifyLock == 0) screen_notifyChange(CHANGE_CONTENT); \
						} while(0)

/** Clear the hanging attribute if the cursor is no longer >= W */
#define clear_invalid_hanging() do { \
									if (cursor.hanging && cursor.x != W-1) cursor.hanging = false; \
								} while(false)

#define cursor_inside_region() (cursor.y >= R0 && cursor.y <= R1)

//region --- Settings ---

/**
 * Restore hard defaults
 */
void ICACHE_FLASH_ATTR
terminal_restore_defaults(void)
{
	termconf->width = SCR_DEF_WIDTH;
	termconf->height = SCR_DEF_HEIGHT;
	termconf->default_bg = 0;
	termconf->default_fg = 7;
	sprintf(termconf->title, SCR_DEF_TITLE);
	for(int i=1; i <= TERM_BTN_COUNT; i++) {
		sprintf(termconf->btn[i-1], "%d", i);
		sprintf(termconf->btn_msg[i-1], "%c", i);
	}
	termconf->theme = 0;
	termconf->parser_tout_ms = SCR_DEF_PARSER_TOUT_MS;
	termconf->display_tout_ms = SCR_DEF_DISPLAY_TOUT_MS;
	termconf->fn_alt_mode = SCR_DEF_FN_ALT_MODE;
	termconf->config_version = TERMCONF_VERSION;
	termconf->display_cooldown_ms = SCR_DEF_DISPLAY_COOLDOWN_MS;
	termconf->loopback = false;
	termconf->show_buttons = true;
	termconf->show_config_links = true;
	termconf->cursor_shape = CURSOR_BLOCK_BL;
	termconf->crlf_mode = false;
}

/**
 * Apply settings after eg. restore from defaults
 */
void ICACHE_FLASH_ATTR
terminal_apply_settings(void)
{
	terminal_apply_settings_noclear();
	screen_init();
}

/** this is used when changing terminal settings that do not affect the screen size */
void ICACHE_FLASH_ATTR
terminal_apply_settings_noclear(void)
{
	bool changed = false;

	// Migrate to v1
	if (termconf->config_version < 1) {
		persist_dbg("termconf: Updating to version 1");
		termconf->display_cooldown_ms = SCR_DEF_DISPLAY_COOLDOWN_MS;
		changed = 1;
	}

	// Migrate to v2
	if (termconf->config_version < 2) {
		persist_dbg("termconf: Updating to version 2");
		termconf->loopback = 0;
		termconf->show_config_links = 1;
		termconf->show_buttons = 1;
		changed = 1;
	}

	// Migrate to v3
	if (termconf->config_version < 3) {
		persist_dbg("termconf: Updating to version 3");
		for(int i=1; i <= TERM_BTN_COUNT; i++) {
			sprintf(termconf->btn_msg[i-1], "%c", i);
		}
		changed = 1;
	}

	// Migrate to v3
	if (termconf->config_version < 4) {
		persist_dbg("termconf: Updating to version 4");
		termconf->cursor_shape = CURSOR_BLOCK_BL;
		termconf->crlf_mode = false;
		changed = 1;
	}

	termconf->config_version = TERMCONF_VERSION;

	// Validation...
	if (termconf->display_tout_ms == 0) {
		termconf->display_tout_ms = SCR_DEF_DISPLAY_TOUT_MS;
		changed = 1;
	}
	if (termconf->display_cooldown_ms == 0) {
		termconf->display_cooldown_ms = SCR_DEF_DISPLAY_COOLDOWN_MS;
		changed = 1;
	}

	memcpy(&termconf_live, termconf, sizeof(TerminalConfigBundle));
	if (W*H > MAX_SCREEN_SIZE) {
		error("BAD SCREEN SIZE: %d rows x %d cols", H, W);
		error("reverting terminal settings to default");
		terminal_restore_defaults();
		changed = true;
		memcpy(&termconf_live, termconf, sizeof(TerminalConfigBundle));
		screen_init();
	}

	if (changed) {
		persist_store();
	}
}
//endregion

//region --- Reset / Init ---

/**
 * Init the screen (entire mappable area - for consistency)
 */
void ICACHE_FLASH_ATTR
screen_init(void)
{
	if(DEBUG_HEAP) dbg("Screen buffer size = %d bytes", sizeof(screen));

	NOTIFY_LOCK();
	Cell sample;
	sample.symbol = ' ';
	sample.fg = termconf->default_fg;
	sample.bg = termconf->default_bg;
	sample.attrs = 0;
	for (unsigned int i = 0; i < MAX_SCREEN_SIZE; i++) {
		memcpy(&screen[i], &sample, sizeof(Cell));
	}

	screen_reset();
	NOTIFY_DONE();
}

/**
 * Reset the cursor position & colors
 */
static void ICACHE_FLASH_ATTR
cursor_reset(void)
{
	cursor.x = 0;
	cursor.y = 0;
	cursor.hanging = false;

	cursor.charsetN = 0;
	cursor.charset0 = CS_B_USASCII;
	cursor.charset1 = CS_0_DEC_SUPPLEMENTAL;

	screen_reset_sgr();
}

/**
 * Reset the screen - called by ESC c
 */
static void ICACHE_FLASH_ATTR
screen_reset_on_resize(void)
{
	ansi_dbg("Screen partial reset due to resize");
	NOTIFY_LOCK();

	cursor.x = 0;
	cursor.y = 0;
	cursor.hanging = false;

	scr.vm0 = 0;
	scr.vm1 = H-1;

	// size is left unchanged
	screen_clear(CLEAR_ALL);
	unicode_cache_clear();

	NOTIFY_DONE();
}

/**
 * Reset the cursor (\e[0m)
 */
void ICACHE_FLASH_ATTR
screen_reset_sgr(void)
{
	cursor.fg = termconf->default_fg;
	cursor.bg = termconf->default_bg;
	cursor.attrs = 0;
	cursor.inverse = false;
	cursor.conceal = false;
}

/**
 * Reset the screen - called by ESC c
 */
void ICACHE_FLASH_ATTR
screen_reset(void)
{
	ansi_dbg("Screen reset.");
	NOTIFY_LOCK();

	cursor_reset();
	unicode_cache_clear();

	// DECopts
	scr.cursor_visible = true;
	scr.insert_mode = false;
	cursor.origin_mode = false;
	cursor.auto_wrap = true;
	cursor.reverse_wrap = false;
	termconf_live.cursor_shape = termconf->cursor_shape;

	scr.numpad_alt_mode = false;
	scr.cursors_alt_mode = false;
	termconf_live.crlf_mode = termconf->crlf_mode;
	scr.reverse_video = false;

	scr.vm0 = 0;
	scr.vm1 = H-1;

	state_backup.alternate_active = false;

	mouse_tracking.encoding = MTE_SIMPLE;
	mouse_tracking.focus_tracking = false;
	mouse_tracking.mode = MTM_NONE;

	// size is left unchanged
	screen_clear(CLEAR_ALL);

	// Set initial tabstops
	for (int i = 0; i < TABSTOP_WORDS; i++) {
		scr.tab_stops[i] = 0x80808080;
	}

	// initial values in the save buffer in case of receiving restore without storing first
	opt_backup.cursors_alt_mode = scr.cursors_alt_mode;
	opt_backup.reverse_video = scr.reverse_video;
	opt_backup.origin_mode = cursor.origin_mode;
	opt_backup.auto_wrap = cursor.auto_wrap;
	opt_backup.mouse_tracking = mouse_tracking.mode;
	opt_backup.mouse_encoding = mouse_tracking.encoding;
	opt_backup.focus_tracking = mouse_tracking.focus_tracking;
	opt_backup.cursor_blink = CURSOR_BLINKS(termconf_live.cursor_shape);
	opt_backup.cursor_visible = scr.cursor_visible;
	opt_backup.reverse_wrap = cursor.reverse_wrap;
	opt_backup.bracketed_paste = scr.bracketed_paste;
	opt_backup.show_buttons = termconf_live.show_buttons;
	opt_backup.show_config_links = termconf_live.show_config_links;

	NOTIFY_DONE();
}

/**
 * Swap screen buffer / state
 * this is CSI ? 47/1047/1049 h/l
 * @param alternate - true if we're swapping, false if unswapping
 */
void ICACHE_FLASH_ATTR
screen_swap_state(bool alternate)
{
	if (alternate == state_backup.alternate_active) {
		ansi_warn("No swap, already alternate = %d", alternate);
		return; // nothing to do
	}

	if (alternate) {
		ansi_dbg("Swap to alternate");
		// store old state
		memcpy(state_backup.title, termconf_live.title, TERM_TITLE_LEN);
		memcpy(state_backup.btn, termconf_live.btn, sizeof(termconf_live.btn));
		memcpy(state_backup.btn_msg, termconf_live.btn_msg, sizeof(termconf_live.btn_msg));
		memcpy(state_backup.tab_stops, scr.tab_stops, sizeof(scr.tab_stops));
		state_backup.vm0 = scr.vm0;
		state_backup.vm1 = scr.vm1;
		// remember old size. may have to resize when returning
		state_backup.width = W;
		state_backup.height = H;
		// TODO backup screen content (if this is ever possible)
	}
	else {
		ansi_dbg("Unswap from alternate");
		NOTIFY_LOCK();
		memcpy(termconf_live.title, state_backup.title, TERM_TITLE_LEN);
		memcpy(termconf_live.btn, state_backup.btn, sizeof(termconf_live.btn));
		memcpy(termconf_live.btn_msg, state_backup.btn_msg, sizeof(termconf_live.btn_msg));
		memcpy(scr.tab_stops, state_backup.tab_stops, sizeof(scr.tab_stops));
		scr.vm0 = state_backup.vm0;
		scr.vm1 = state_backup.vm1;
		// this may clear the screen as a side effect if size changed
		screen_resize(state_backup.height, state_backup.width);
		// TODO restore screen content (if this is ever possible)
		NOTIFY_DONE();
		screen_notifyChange(CHANGE_LABELS);
	}

	state_backup.alternate_active = alternate;
}

//endregion

//region --- Tab stops ---

void ICACHE_FLASH_ATTR
screen_clear_all_tabs(void)
{
	memset(scr.tab_stops, 0, sizeof(scr.tab_stops));
}

void ICACHE_FLASH_ATTR
screen_set_tab(void)
{
	scr.tab_stops[cursor.x/32] |= (1<<(cursor.x%32));
}

void ICACHE_FLASH_ATTR
screen_clear_tab(void)
{
	scr.tab_stops[cursor.x/32] &= ~(1<<(cursor.x%32));
}

/**
 * Find tab stop closest to cursor to the right
 * @return X pos or -1
 */
static int ICACHE_FLASH_ATTR
next_tab_stop(void)
{
	// cursor must never go past EOL
	if (cursor.x >= W-1) return -1;

	// find first word to inspect
	int idx = (cursor.x+1)/32;
	int offs = (cursor.x+1)%32;
	int cp = cursor.x;
	while (idx < TABSTOP_WORDS) {
		u32 w = scr.tab_stops[idx];
		w >>= offs;
		for(;offs<32;offs++) {
			cp++;
			if (cp >= W) return -1;
			if (w & 1) return cp;
			w >>= 1;
		}
		offs = 0;
		idx++;
	}

	return -1;
}

/**
 * Find tab stop closest to cursor to the left
 * @return X pos or -1
 */
static int ICACHE_FLASH_ATTR
prev_tab_stop(void)
{
	// nowhere to go
	if (cursor.x == 0) return -1;

	// find first word to inspect
	int idx = (cursor.x-1)/32;
	int offs = (cursor.x-1)%32;
	int cp = cursor.x;
	while (idx >= 0) {
		u32 w = scr.tab_stops[idx];
		w <<= 31-offs;
		if (w == 0) {
			cp -= cp%32;
			if (cp < 0) return -1;
			goto next;
		}
		for(;offs>=0;offs--) {
			cp--;
			if (cp < 0) return -1;
			if (w & (1<<31)) return cp;
			w <<= 1;
		}
		next:
		offs = 31;
		idx--;
	}

	return -1;
}

void ICACHE_FLASH_ATTR
screen_tab_forward(int count)
{
	NOTIFY_LOCK();
	for (; count > 0; count--) {
		int tab = next_tab_stop();
		if (tab != -1) {
			cursor.x = tab;
		}
		else {
			cursor.x = W - 1;
		}
	}
	NOTIFY_DONE();
}

void ICACHE_FLASH_ATTR
screen_tab_reverse(int count)
{
	NOTIFY_LOCK();
	for (; count > 0; count--) {
		int tab = prev_tab_stop();
		if (tab != -1) {
			cursor.x = tab;
		} else {
			cursor.x = 0;
		}
	}
	NOTIFY_DONE();
}

//endregion

//region --- Clearing & inserting ---

/**
 * Clear range, inclusive
 */
static void ICACHE_FLASH_ATTR
clear_range(unsigned int from, unsigned int to)
{
	if (to >= W*H) to = W*H-1;
	Color fg = (cursor.inverse) ? cursor.bg : cursor.fg;
	Color bg = (cursor.inverse) ? cursor.fg : cursor.bg;

	Cell sample;
	sample.symbol = ' ';
	sample.fg = fg;
	sample.bg = bg;
	sample.attrs = 0;

	for (unsigned int i = from; i <= to; i++) {
		UnicodeCacheRef symbol = screen[i].symbol;
		if (IS_UNICODE_CACHE_REF(symbol)) unicode_cache_remove(symbol);
		memcpy(&screen[i], &sample, sizeof(Cell));
	}
}

/**
 * Clear screen area
 */
void ICACHE_FLASH_ATTR
screen_clear(ClearMode mode)
{
	// Those ignore margins and DECOM
	NOTIFY_LOCK();
	switch (mode) {
		case CLEAR_ALL:
			clear_range(0, W * H - 1);
			break;

		case CLEAR_FROM_CURSOR:
			clear_range((cursor.y * W) + cursor.x, W * H - 1);
			break;

		case CLEAR_TO_CURSOR:
			clear_range(0, (cursor.y * W) + cursor.x);
			break;
	}
	NOTIFY_DONE();
}

/**
 * Line reset to gray-on-white, empty
 */
void ICACHE_FLASH_ATTR
screen_clear_line(ClearMode mode)
{
	NOTIFY_LOCK();
	switch (mode) {
		case CLEAR_ALL:
			clear_range(cursor.y * W, (cursor.y + 1) * W - 1);
			break;

		case CLEAR_FROM_CURSOR:
			clear_range(cursor.y * W + cursor.x, (cursor.y + 1) * W - 1);
			break;

		case CLEAR_TO_CURSOR:
			clear_range(cursor.y * W, cursor.y * W + cursor.x);
			break;
	}
	NOTIFY_DONE();
}

void ICACHE_FLASH_ATTR
screen_clear_in_line(unsigned int count)
{
	if (cursor.x + count >= W) {
		screen_clear_line(CLEAR_FROM_CURSOR);
	}
	else {
		NOTIFY_LOCK();
		clear_range(cursor.y * W + cursor.x, cursor.y * W + cursor.x + count - 1);
		NOTIFY_DONE();
	}
}

void ICACHE_FLASH_ATTR
screen_insert_lines(unsigned int lines)
{
	if (!cursor_inside_region()) return; // can't insert if not inside region
	NOTIFY_LOCK();

	// FIXME remove cleared & overwritten cells from unicode cache!

	// shift the following lines
	int targetStart = cursor.y + lines;
	if (targetStart > R1) {
		targetStart = R1-1;
	} else {
		// do the moving
		for (int i = R1; i >= targetStart; i--) {
			memcpy(screen+i*W, screen+(i-lines)*W, sizeof(Cell)*W);
		}
	}

	// clear the first line
	screen_clear_line(CLEAR_ALL);
	// copy it to the rest of the cleared region
	for (int i = cursor.y+1; i < targetStart; i++) {
		memcpy(screen+i*W, screen+cursor.y*W, sizeof(Cell)*W);
	}
	NOTIFY_DONE();
}

void ICACHE_FLASH_ATTR
screen_delete_lines(unsigned int lines)
{
	if (!cursor_inside_region()) return; // can't delete if not inside region
	NOTIFY_LOCK();

	// FIXME remove cleared & overwritten cells from unicode cache!

	// shift lines up
	int targetEnd = R1 - lines - 1;
	if (targetEnd <= cursor.y) {
		targetEnd = cursor.y;
	} else {
		// do the moving
		for (int i = cursor.y; i <= targetEnd; i++) {
			memcpy(screen+i*W, screen+(i+lines)*W, sizeof(Cell)*W);
		}
	}

	// clear the rest
	clear_range(targetEnd*W, W*R1);
	NOTIFY_DONE();
}

void ICACHE_FLASH_ATTR
screen_insert_characters(unsigned int count)
{
	NOTIFY_LOCK();

	// FIXME remove cleared & overwritten cells from unicode cache!

	int targetStart = cursor.x + count;
	if (targetStart >= W) {
		targetStart = W-1;
	} else {
		// do the moving
		for (int i = W-1; i >= targetStart; i--) {
			memcpy(screen+cursor.y*W+i, screen+cursor.y*W+(i-count), sizeof(Cell));
		}
	}

	clear_range(cursor.y*W+cursor.x, cursor.y*W+targetStart-1);
	NOTIFY_DONE();
}

void ICACHE_FLASH_ATTR
screen_delete_characters(unsigned int count)
{
	NOTIFY_LOCK();

	// FIXME remove cleared & overwritten cells from unicode cache!
	int targetEnd = W - count;
	if (targetEnd > cursor.x) {
		// do the moving
		for (int i = cursor.x; i <= targetEnd; i++) {
			memcpy(screen+cursor.y*W+i, screen+cursor.y*W+(i+count), sizeof(Cell));
		}
	}

	if (targetEnd <= cursor.x) {
		screen_clear_line(CLEAR_FROM_CURSOR);
	}
	else {
		clear_range(cursor.y * W + (W - count), (cursor.y + 1) * W - 1);
	}
	NOTIFY_DONE();
}
//endregion

//region --- Entire screen manipulation ---

void ICACHE_FLASH_ATTR
screen_fill_with_E(void)
{
	NOTIFY_LOCK();
	screen_reset(); // based on observation from xterm

	Cell sample;
	sample.symbol = 'E';
	sample.fg = termconf->default_fg;
	sample.bg = termconf->default_bg;
	sample.attrs = 0;

	for (unsigned int i = 0; i <= W*H-1; i++) {
		memcpy(&screen[i], &sample, sizeof(Cell));
	}
	NOTIFY_DONE();
}

/**
 * Change the screen size
 *
 * @param cols - new width
 * @param rows - new height
 */
void ICACHE_FLASH_ATTR
screen_resize(int rows, int cols)
{
	// sanitize
	if (cols < 1 || rows < 1) {
		error("Bad size: %d x %d", cols, rows);
		return;
	}

	if (cols * rows > MAX_SCREEN_SIZE) {
		error("Too big size: %d x %d", cols, rows);
		return;
	}

	if (W == cols && H == rows) return; // Do nothing

	NOTIFY_LOCK();
	W = cols;
	H = rows;
	screen_reset_on_resize();
	NOTIFY_DONE();
}

void ICACHE_FLASH_ATTR
screen_set_title(const char *title)
{
	strncpy(termconf_live.title, title, TERM_TITLE_LEN);
	screen_notifyChange(CHANGE_LABELS);
}

/**
 * Helper function to set terminal button label
 * @param num - button number 1-5
 * @param str - button text
 */
void ICACHE_FLASH_ATTR
screen_set_button_text(int num, const char *text)
{
	strncpy(termconf_live.btn[num-1], text, TERM_BTN_LEN);
	screen_notifyChange(CHANGE_LABELS);
}

/**
 * Shift screen upwards
 */
void ICACHE_FLASH_ATTR
screen_scroll_up(unsigned int lines)
{
	NOTIFY_LOCK();
	if (lines >= RH) {
		clear_range(R0*W, (R1+1)*W-1);
		goto done;
	}

	// bad cmd
	if (lines == 0) {
		goto done;
	}

	// FIXME remove cleared & overwritten cells from unicode cache!

	int y;
	for (y = R0; y <= R1 - lines; y++) {
		memcpy(screen + y * W, screen + (y + lines) * W, W * sizeof(Cell));
	}

	clear_range(y * W, (R1+1)*W-1);

	done:
	NOTIFY_DONE();
}

/**
 * Shift screen downwards
 */
void ICACHE_FLASH_ATTR
screen_scroll_down(unsigned int lines)
{
	NOTIFY_LOCK();
	if (lines >= RH) {
		clear_range(R0*W, (R1+1)*W-1);
		goto done;
	}

	// FIXME remove cleared & overwritten cells from unicode cache!

	// bad cmd
	if (lines == 0) {
		goto done;
	}

	int y;
	for (y = R1; y >= R0+lines; y--) {
		memcpy(screen + y * W, screen + (y - lines) * W, W * sizeof(Cell));
	}

	clear_range(R0*W, R0*W+ lines * W-1);
	done:
	NOTIFY_DONE();
}

/** Set scrolling region */
void ICACHE_FLASH_ATTR
screen_set_scrolling_region(int from, int to)
{
	if (from <= 0 && to <= 0) {
		scr.vm0 = 0;
		scr.vm1 = H-1;
	} else if (from >= 0 && from < H-1 && to > from) {
		scr.vm0 = from;
		scr.vm1 = to;
	} else {
		// Bad range, do nothing
		ansi_warn("Bad SR bounds %d, %d", from, to);
	}

	// Always move cursor home (may be translated due to DECOM)
	screen_cursor_set(0, 0);
}

//endregion

//region --- Cursor manipulation ---

/** set cursor type */
void ICACHE_FLASH_ATTR
screen_cursor_shape(enum CursorShape shape)
{
	NOTIFY_LOCK();
	if (shape == CURSOR_DEFAULT) shape = termconf->cursor_shape;
	termconf_live.cursor_shape = shape;
	NOTIFY_DONE();
}

/** set cursor blink option */
void ICACHE_FLASH_ATTR
screen_cursor_blink(bool blink)
{
	NOTIFY_LOCK();
	if (blink) {
		if (termconf_live.cursor_shape == CURSOR_BLOCK) termconf_live.cursor_shape = CURSOR_BLOCK_BL;
		if (termconf_live.cursor_shape == CURSOR_BAR) termconf_live.cursor_shape = CURSOR_BAR_BL;
		if (termconf_live.cursor_shape == CURSOR_UNDERLINE) termconf_live.cursor_shape = CURSOR_UNDERLINE_BL;
	} else {
		if (termconf_live.cursor_shape == CURSOR_BLOCK_BL) termconf_live.cursor_shape = CURSOR_BLOCK;
		if (termconf_live.cursor_shape == CURSOR_BAR_BL) termconf_live.cursor_shape = CURSOR_BAR;
		if (termconf_live.cursor_shape == CURSOR_UNDERLINE_BL) termconf_live.cursor_shape = CURSOR_UNDERLINE;
	}
	NOTIFY_DONE();
}

/**
 * Set cursor position
 */
void ICACHE_FLASH_ATTR
screen_cursor_set(int y, int x)
{
	NOTIFY_LOCK();
	screen_cursor_set_x(x);
	screen_cursor_set_y(y);
	NOTIFY_DONE();
}

/**
 * Set cursor position
 */
void ICACHE_FLASH_ATTR
screen_cursor_get(int *y, int *x)
{
	*x = cursor.x;
	*y = cursor.y;

	if (cursor.origin_mode) {
		*y -= R0;
	}
}

/**
 * Set cursor X position
 */
void ICACHE_FLASH_ATTR
screen_cursor_set_x(int x)
{
	NOTIFY_LOCK();
	if (x >= W) x = W - 1;
	if (x < 0) x = 0;
	cursor.x = x;
	// Always clear hanging on cursor set
	// hanging happens when the cursor is virtually at col=81, which
	// cannot be set using the cursor-set commands.
	cursor.hanging = false;
	NOTIFY_DONE();
}

/**
 * Set cursor Y position
 */
void ICACHE_FLASH_ATTR
screen_cursor_set_y(int y)
{
	NOTIFY_LOCK();
	if (cursor.origin_mode) {
		y += R0;
		if (y > R1) y = R1;
		if (y < R0) y = R0;
	} else {
		if (y > H-1) y = H-1;
		if (y < 0) y = 0;
	}
	cursor.y = y;
	NOTIFY_DONE();
}

/**
 * Relative cursor move
 */
void ICACHE_FLASH_ATTR
screen_cursor_move(int dy, int dx, bool scroll)
{
	NOTIFY_LOCK();
	int move;

	clear_invalid_hanging();

	if (cursor.hanging && dx < 0) {
		dx += 1; // consume one step on the removal of "xenl"
		cursor.hanging = false;
	}

	bool was_inside = cursor_inside_region();

	cursor.x += dx;
	cursor.y += dy;
	if (cursor.x >= (int)W) cursor.x = W - 1;
	if (cursor.x < (int)0) {
		if (cursor.auto_wrap && cursor.reverse_wrap) {
			// this is mimicking a behavior from xterm that allows any number of steps backwards with reverse wraparound enabled
			int steps = -cursor.x;
			if(steps > W*H) steps = W*H; // avoid something stupid causing infinite loop here
			for(;steps>0;steps--) {
				if (cursor.x > 0) {
					// backspace should go to col 79 if "hanging" after 80 (as if it never actually left the 80th col)
					cursor.hanging = false;
					cursor.x--;
				}
				else {
					if (cursor.y > 0) {
						// end of previous line
						cursor.y--;
						cursor.x = W - 1;
					}
					else {
						// end of screen, end of line (wrap around)
						cursor.y = R1;
						cursor.x = W - 1;
					}
				}
			}
		} else {
			cursor.x = 0;
		}
	}

	if (cursor.y < R0) {
		if (was_inside) {
			move = -(cursor.y - R0);
			cursor.y = R0;
			if (scroll) screen_scroll_down((unsigned int) move);
		}
		else {
			// outside the region, just validate that we're not going offscreen
			// scrolling is not possible in this case
			if (cursor.y < 0) {
				cursor.y = 0;
			}
		}
	}

	if (cursor.y > R1) {
		if (was_inside) {
			move = cursor.y - R1;
			cursor.y = R1;
			if (scroll) screen_scroll_up((unsigned int) move);
		}
		else {
			// outside the region, just validate that we're not going offscreen
			// scrolling is not possible in this case
			if (cursor.y >= H) {
				cursor.y = H-1;
			}
		}
	}

	NOTIFY_DONE();
}

/**
 * Save the cursor pos
 */
void ICACHE_FLASH_ATTR
screen_cursor_save(bool withAttrs)
{
	(void)withAttrs;
	// always save with attribs
	memcpy(&cursor_sav, &cursor, sizeof(CursorTypeDef));
	cursor_saved = true;
}

/**
 * Restore the cursor pos
 */
void ICACHE_FLASH_ATTR
screen_cursor_restore(bool withAttrs)
{
	NOTIFY_LOCK();

	if (!cursor_saved) {
		cursor_reset();
	}
	else {
		if (withAttrs) {
			memcpy(&cursor, &cursor_sav, sizeof(CursorTypeDef));
		}
		else {
			cursor.x = cursor_sav.x;
			cursor.y = cursor_sav.y;
			cursor.hanging = cursor_sav.hanging;
		}
	}

	NOTIFY_DONE();
}

void ICACHE_FLASH_ATTR
screen_back_index(int count)
{
	NOTIFY_LOCK();
	int new_x = cursor.x - count;
	if (new_x >= 0) {
		cursor.x = new_x;
	} else {
		cursor.x = 0;
		screen_insert_characters(-new_x);
	}
	NOTIFY_DONE();
}

//endregion

//region --- Attribute setting ---

/**
 * Enable cursor display
 */
void ICACHE_FLASH_ATTR
screen_set_cursor_visible(bool visible)
{
	NOTIFY_LOCK();
	scr.cursor_visible = visible;
	NOTIFY_DONE();
}

/**
 * Enable autowrap
 */
void ICACHE_FLASH_ATTR
screen_wrap_enable(bool enable)
{
	cursor.auto_wrap = enable;
}

/**
 * Enable reverse wraparound
 */
void ICACHE_FLASH_ATTR
screen_reverse_wrap_enable(bool enable)
{
	cursor.reverse_wrap = enable;
}

/**
 * Set cursor foreground color
 */
void ICACHE_FLASH_ATTR
screen_set_fg(Color color)
{
	if (color > COLOR_MAX) color = COLOR_MAX;
	cursor.fg = color;
}

/**
 * Set cursor background coloor
 */
void ICACHE_FLASH_ATTR
screen_set_bg(Color color)
{
	if (color > COLOR_MAX) color = COLOR_MAX;
	cursor.bg = color;
}

/**
 * Set cursor foreground color, extended
 */
void ICACHE_FLASH_ATTR
screen_set_fg_ext(u16 color)
{
	// TODO validate and set
}

/**
 * Set cursor background color, extended
 */
void ICACHE_FLASH_ATTR
screen_set_bg_ext(u16 color)
{
	// TODO validate and set
}

void ICACHE_FLASH_ATTR
screen_set_sgr(u8 attrs, bool ena)
{
	if (ena) {
		cursor.attrs |= attrs;
	}
	else {
		cursor.attrs &= ~attrs;
	}
}

void ICACHE_FLASH_ATTR
screen_set_sgr_inverse(bool ena)
{
	cursor.inverse = ena;
}

void ICACHE_FLASH_ATTR
screen_set_sgr_conceal(bool ena)
{
	cursor.conceal = ena;
}

void ICACHE_FLASH_ATTR
screen_set_charset_n(int Gx)
{
	if (Gx < 0 || Gx > 1) return; // bad n
	cursor.charsetN = Gx;
}

void ICACHE_FLASH_ATTR
screen_set_charset(int Gx, char charset)
{
	if (Gx == 0) cursor.charset0 = charset;
	else if (Gx == 1) cursor.charset1 = charset;
}

void ICACHE_FLASH_ATTR
screen_set_insert_mode(bool insert)
{
	scr.insert_mode = insert;
}

void ICACHE_FLASH_ATTR
screen_set_numpad_alt_mode(bool alt_mode)
{
	NOTIFY_LOCK();
	scr.numpad_alt_mode = alt_mode;
	NOTIFY_DONE();
}

void ICACHE_FLASH_ATTR
screen_set_cursors_alt_mode(bool alt_mode)
{
	NOTIFY_LOCK();
	scr.cursors_alt_mode = alt_mode;
	NOTIFY_DONE();
}

void ICACHE_FLASH_ATTR
screen_set_reverse_video(bool reverse)
{
	NOTIFY_LOCK();
	scr.reverse_video = reverse;
	NOTIFY_DONE();
}

void ICACHE_FLASH_ATTR
screen_set_newline_mode(bool nlm)
{
	NOTIFY_LOCK();
	termconf_live.crlf_mode = nlm;
	NOTIFY_DONE();
}

void ICACHE_FLASH_ATTR
screen_set_origin_mode(bool region_origin)
{
	cursor.origin_mode = region_origin;
	screen_cursor_set(0, 0);
}

static void ICACHE_FLASH_ATTR
do_save_private_opt(int n, bool save)
{
#define SAVE_RESTORE(sf, of) do { if (save) sf=(of); else of=(sf); } while(0)
	switch (n) {
		case 1:
			SAVE_RESTORE(opt_backup.cursors_alt_mode, scr.cursors_alt_mode);
			break;
		case 5:
			SAVE_RESTORE(opt_backup.reverse_video, scr.reverse_video);
			break;
		case 6:
			SAVE_RESTORE(opt_backup.origin_mode, cursor.origin_mode);
			break;
		case 7:
			SAVE_RESTORE(opt_backup.auto_wrap, cursor.auto_wrap);
			break;
		case 9:
		case 1000:
		case 1001: // hilite, not implemented
		case 1002:
		case 1003:
			SAVE_RESTORE(opt_backup.mouse_tracking, mouse_tracking.mode);
			break;
		case 1004:
			SAVE_RESTORE(opt_backup.focus_tracking, mouse_tracking.focus_tracking);
			break;
		case 1005:
		case 1006:
		case 1015:
			SAVE_RESTORE(opt_backup.mouse_encoding, mouse_tracking.encoding);
			break;
		case 12: // cursor blink
			if (save) {
				opt_backup.cursor_blink = CURSOR_BLINKS(termconf_live.cursor_shape);
			} else {
				screen_cursor_blink(opt_backup.cursor_blink);
			}
			break;
		case 25:
			SAVE_RESTORE(opt_backup.cursor_visible, scr.cursor_visible);
			break;
		case 45:
			SAVE_RESTORE(opt_backup.reverse_wrap, cursor.reverse_wrap);
			break;
		case 2004:
			SAVE_RESTORE(opt_backup.bracketed_paste, scr.bracketed_paste);
			break;
		case 800:
			SAVE_RESTORE(opt_backup.show_buttons, termconf_live.show_buttons);
			break;
		case 801:
			SAVE_RESTORE(opt_backup.show_config_links, termconf_live.show_config_links);
			break;
		default:
			ansi_warn("Cannot store ?%d", n);
	}
}

void ICACHE_FLASH_ATTR
screen_save_private_opt(int n)
{
	do_save_private_opt(n, true);
}

void ICACHE_FLASH_ATTR
screen_restore_private_opt(int n)
{
	NOTIFY_LOCK();
	do_save_private_opt(n, false);
	NOTIFY_DONE();
}

void ICACHE_FLASH_ATTR
screen_report_sgr(char *buffer)
{
	buffer += sprintf(buffer, "0");
	if (cursor.attrs & ATTR_BOLD) buffer += sprintf(buffer, ";%d", SGR_BOLD);
	if (cursor.attrs & ATTR_FAINT) buffer += sprintf(buffer, ";%d", SGR_FAINT);
	if (cursor.attrs & ATTR_ITALIC) buffer += sprintf(buffer, ";%d", SGR_ITALIC);
	if (cursor.attrs & ATTR_UNDERLINE) buffer += sprintf(buffer, ";%d", SGR_UNDERLINE);
	if (cursor.attrs & ATTR_BLINK) buffer += sprintf(buffer, ";%d", SGR_BLINK);
	if (cursor.attrs & ATTR_FRAKTUR) buffer += sprintf(buffer, ";%d", SGR_FRAKTUR);
	if (cursor.attrs & ATTR_STRIKE) buffer += sprintf(buffer, ";%d", SGR_STRIKE);
	if (cursor.inverse) buffer += sprintf(buffer, ";%d", SGR_INVERSE);
	if (cursor.fg != termconf->default_fg) buffer += sprintf(buffer, ";%d", ((cursor.fg > 7) ? SGR_FG_BRT_START : SGR_FG_START) + (cursor.fg&7));
	if (cursor.bg != termconf->default_bg) buffer += sprintf(buffer, ";%d", ((cursor.bg > 7) ? SGR_BG_BRT_START : SGR_BG_START) + (cursor.bg&7));
	(void)buffer;
}

//endregion

//region --- Printing ---

static const char *putchar_graphic(const char *ch)
{
	static char buf[4];

	if (cursor.hanging) {
		// perform the scheduled wrap if hanging
		// if auto-wrap = off, it overwrites the last char
		if (cursor.auto_wrap) {
			cursor.x = 0;
			cursor.y++;
			// Y wrap
			if (cursor.y > R1) {
				// Scroll up, so we have space for writing
				screen_scroll_up(1);
				cursor.y = R1;
			}

			cursor.hanging = false;
		}
	}

	Cell *c = &screen[cursor.x + cursor.y * W];

	// move the rest of the line if we're in Insert Mode
	if (cursor.x < W-1 && scr.insert_mode) screen_insert_characters(1);

	if (ch[1] == 0 && ch[0] <= 0x7f) {
		// we have len=1 and ASCII, can be re-mapped using a table
		utf8_remap(buf, ch[0], (cursor.charsetN == 0) ? cursor.charset0 : cursor.charset1);
		ch = buf;
	}
	unicode_cache_remove(c->symbol);
	c->symbol = unicode_cache_add((const u8 *)ch);

	if (cursor.inverse) {
		c->fg = cursor.bg;
		c->bg = cursor.fg;
	} else {
		c->fg = cursor.fg;
		c->bg = cursor.bg;
	}
	c->attrs = cursor.attrs;

	cursor.x++;
	// X wrap
	if (cursor.x >= W) {
		cursor.hanging = true; // hanging - next typed char wraps around, but backspace and arrows still stay on the same line.
		cursor.x = W - 1;
	}

	return ch;
}

/**
 * Set a character in the cursor color, move to right with wrap.
 */
void ICACHE_FLASH_ATTR
screen_putchar(const char *ch)
{
	NOTIFY_LOCK();

	// clear "hanging" flag if not possible
	clear_invalid_hanging();

	if (cursor.conceal) {
		ch = " ";
	}

	// Special treatment for CRLF
	switch (ch[0]) {
		case DEL:
			goto done;

		case CR:
			screen_cursor_set_x(0);
			goto done;

		case LF:
			screen_cursor_move(1, 0, true); // can scroll
			goto done;

		case BS:
			screen_cursor_move(0, -1, false);
			// we should not wrap around, and backspace should not even clear the cell (verified in xterm)
			goto done;

		default:
			if (ch[0] < SP) {
				// Discard
				warn("Ignoring control char %d", (int)ch[0]);
				goto done;
			}
	}

	const char *result = putchar_graphic(ch);

	// Remember the resulting character
	// If we re-mapped ASCII to high UTF, the following Repeat command will
	// not have to call the remap function repeatedly.
	strncpy(scr.last_char, result, 4);

	done:
	NOTIFY_DONE();
}

/**
 * Repeat last graphic character
 * @param count
 */
void ICACHE_FLASH_ATTR
screen_repeat_last_character(int count)
{
	NOTIFY_LOCK();
	if (scr.last_char[0]==0) {
		scr.last_char[0] = ' ';
		scr.last_char[1] = 0;
	}

	if (count > W*H) count = W*H;

	while (count > 0) {
		putchar_graphic(scr.last_char);
		count--;
	}
	NOTIFY_DONE();
}

/**
 * UTF remap
 * @param out - output char[4]
 * @param g  - ASCII char
 * @param charset - table name (0, A, B)
 */
static void ICACHE_FLASH_ATTR
utf8_remap(char *out, char g, char charset)
{
	u16 n;
	u16 utf = (unsigned char)g;

	switch (charset) {
		case CS_0_DEC_SUPPLEMENTAL: /* DEC Special Character & Line Drawing Set */
			if ((g >= CODEPAGE_0_BEGIN) && (g <= CODEPAGE_0_END)) {
				n = codepage_0[g - CODEPAGE_0_BEGIN];
				if (n) utf = n;
			}
			break;

		case CS_1_DOS_437: /* ESPTerm Character Rom 1 */
			if ((g >= CODEPAGE_1_BEGIN) && (g <= CODEPAGE_1_END)) {
				n = codepage_1[g - CODEPAGE_1_BEGIN];
				if (n) utf = n;
			}
			break;

		case CS_A_UKASCII: /* UK, replaces # with GBP */
			if (g == '#') utf = 0x20a4; // £
			break;

		default:
		case CS_B_USASCII:
			// No change
			break;
	}

	utf8_encode(out, utf);
}
//endregion

//region --- Serialization ---

struct ScreenSerializeState {
	Color lastFg;
	Color lastBg;
	bool lastAttrs;
	UnicodeCacheRef lastSymbol;
	char lastChar[4];
	u8 lastCharLen;
	int index;
};

/**
 * buffer should be at least 64+5*10+6 long (title + buttons + 6), ie. 120
 * @param buffer
 * @param buf_len
 */
void ICACHE_FLASH_ATTR
screenSerializeLabelsToBuffer(char *buffer, size_t buf_len)
{
	// let's just assume it's long enough - called with the huge websocket buffer
	sprintf(buffer, "T%s\x01%s\x01%s\x01%s\x01%s\x01%s", // use 0x01 as separator
			termconf_live.title,
			termconf_live.btn[0],
			termconf_live.btn[1],
			termconf_live.btn[2],
			termconf_live.btn[3],
			termconf_live.btn[4]
	);
}

/**
 * Serialize the screen to a data buffer. May need multiple calls if the buffer is insufficient in size.
 *
 * @warning MAKE SURE *DATA IS NULL BEFORE FIRST CALL!
 *          Call with NULL 'buffer' at the end to free the data struct.
 *
 * @param buffer - buffer array of limited size. If NULL, indicates this is the last call.
 * @param buf_len - buffer array size
 * @param data - opaque pointer to internal data structure for storing state between repeated calls
 *               if NULL, indicates this is the first call.
 * @return HTTPD_CGI_DONE or HTTPD_CGI_MORE. If more, repeat with the same DATA.
 */
httpd_cgi_state ICACHE_FLASH_ATTR
screenSerializeToBuffer(char *buffer, size_t buf_len, void **data)
{
	struct ScreenSerializeState *ss = *data;

	if (buffer == NULL) {
		if (ss != NULL) free(ss);
		return HTTPD_CGI_DONE;
	}

	Cell *cell, *cell0;
	WordB2 w1;
	WordB3 lw1;

	size_t remain = buf_len;
	char *bb = buffer;

#define bufput_c(c) do { \
			*bb = (char)c; bb++; \
			remain--; \
		} while(0)

#define bufput_2B(n) do { \
			encode2B((u16) n, &w1); \
			bufput_c(w1.lsb); \
			bufput_c(w1.msb); \
		} while(0)

#define bufput_3B(n) do { \
			encode3B((u32) n, &lw1); \
			bufput_c(lw1.lsb); \
			bufput_c(lw1.msb); \
			bufput_c(lw1.xsb); \
		} while(0)

#define bufput_t2B(t, n) do { \
			bufput_c(t); \
			bufput_2B(n); \
		} while(0)

#define bufput_t3B(t, n) do { \
			bufput_c(t); \
			bufput_3B(n); \
		} while(0)

	if (ss == NULL) {
		*data = ss = malloc(sizeof(struct ScreenSerializeState));
		ss->index = 0;
		ss->lastBg = 0;
		ss->lastFg = 0;
		ss->lastAttrs = 0;
		ss->lastCharLen = 0;
		ss->lastSymbol = 32;
		strncpy(ss->lastChar, " ", 4);

		bufput_c('S');
		// H W X Y Attribs
		bufput_2B(H);
		bufput_2B(W);
		bufput_2B(cursor.y);
		bufput_2B(cursor.x);
		// 3B has 18 free bits
		bufput_3B(
			(scr.cursor_visible << 0) |
			(cursor.hanging << 1) |
			(scr.cursors_alt_mode << 2) |
			(scr.numpad_alt_mode << 3) |
			(termconf_live.fn_alt_mode << 4) |
			((mouse_tracking.mode>MTM_NONE) << 5) | // disables context menu
			((mouse_tracking.mode>=MTM_NORMAL) << 6) | // disables selecting
			(termconf_live.show_buttons << 7) |
			(termconf_live.show_config_links << 8) |
			((termconf_live.cursor_shape&0x07) << 9) | // 9,10,11 - cursor shape based on DECSCUSR
		    (termconf_live.crlf_mode << 12)
	    );
	}

	int i = ss->index;
	while(i < W*H && remain > 12) {
		cell = cell0 = &screen[i];

		// Count how many times same as previous
		int repCnt = 0;
		while (i < W*H
			   && cell->fg == ss->lastFg
			   && cell->bg == ss->lastBg
			   && cell->attrs == ss->lastAttrs
			   && cell->symbol == ss->lastSymbol) {
			// Repeat
			repCnt++;
			cell = &screen[++i];
		}

		if (repCnt == 0) {
			// No repeat
			bool changeAttrs = cell0->attrs != ss->lastAttrs;
			bool changeColors = (cell0->fg != ss->lastFg || cell0->bg != ss->lastBg);
			Color fg, bg;

			// Reverse fg and bg if we're in global reverse mode
			if (! scr.reverse_video) {
				fg = cell0->fg;
				bg = cell0->bg;
			}
			else {
				fg = cell0->bg;
				bg = cell0->fg;
			}

			if (!changeAttrs && changeColors) {
				bufput_t2B('\x03', fg | (bg<<4));
			}
			else if (changeAttrs && !changeColors) {
				// attrs only
				bufput_t2B('\x04', cell0->attrs);
			}
			else if (changeAttrs && changeColors) {
				// colors and attrs
				bufput_t3B('\x01', (u32) (fg | (bg<<4) | (cell0->attrs<<8)));
			}

			// copy the symbol, until first 0 or reached 4 bytes
			char c;
			ss->lastCharLen = 0;
			unicode_cache_retrieve(cell->symbol, (u8 *) ss->lastChar);
			for(int j=0; j<4; j++) {
				c = ss->lastChar[j];
				if(!c) break;
				bufput_c(c);
				ss->lastCharLen++;
			}

			ss->lastFg = cell0->fg;
			ss->lastBg = cell0->bg;
			ss->lastAttrs = cell0->attrs;
			ss->lastSymbol = cell0->symbol;

			i++;
		} else {
			// last character was repeated repCnt times
			int savings = ss->lastCharLen*repCnt;
			if (savings > 3) {
				// Repeat count
				bufput_t2B('\x02', repCnt);
			} else {
				// repeat it manually
				for(int k = 0; k < repCnt; k++) {
					for (int j = 0; j < ss->lastCharLen; j++) {
						bufput_c(ss->lastChar[j]);
					}
				}
			}
		}
	}

	ss->index = i;
	bufput_c('\0'); // terminate the string

	if (i < W*H-1) {
		return HTTPD_CGI_MORE;
	}

	return HTTPD_CGI_DONE;
}
//endregion
