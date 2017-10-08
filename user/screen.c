#include <esp8266.h>
#include <httpd.h>
#include "screen.h"
#include "persist.h"
#include "sgr.h"
#include "ascii.h"
#include "apars_logging.h"
#include "character_sets.h"
#include "utf8.h"
#include "cgi_sockets.h"

TerminalConfigBundle * const termconf = &persist.current.termconf;
TerminalConfigBundle termconf_live;

MouseTrackingConfig mouse_tracking;

// forward declare
static void utf8_remap(char* out, char g, char charset);

#define W termconf_live.width
#define H termconf_live.height

/**
 * Screen cell data type (16 bits)
 */
typedef struct __attribute__((packed)) {
	UnicodeCacheRef symbol : 8;
	Color fg;
	Color bg;
	CellAttrs attrs;
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

#define TOP scr.vm0
#define BTM scr.vm1
#define RH (scr.vm1 - scr.vm0 + 1)
// horizontal edges - will be useful if horizontal margin is implemented
//#define C0 0
//#define C1 (W-1)

typedef struct {
	/* Cursor position */
	int x;        //!< X coordinate
	int y;        //!< Y coordinate
	bool hanging; //!< xenl state - cursor half-wrapped

	/* SGR */
	bool conceal; //!< similar to inverse, causes all to be replaced by SP
	u16 attrs;
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
static volatile ScreenNotifyTopics lockTopics = 0;

static struct {
	int x_min, y_min, x_max, y_max;
} scr_dirty;

#define reset_screen_dirty() do { \
		scr_dirty.x_min = W; \
		scr_dirty.x_max = -1; \
		scr_dirty.y_min = H; \
		scr_dirty.y_max = -1; \
	} while(0)

#define expand_dirty(y0, y1, x0, x1)  do { \
		seri_dbg("Expand: X: (%d..%d) -> %d..%d, Y: (%d..%d) -> %d..%d", scr_dirty.x_min, scr_dirty.x_max, x0, x1, scr_dirty.y_min, scr_dirty.y_max, y0, y1); \
		if ((int)(y0) < scr_dirty.y_min) scr_dirty.y_min  = (y0); \
		if ((int)(x0) < scr_dirty.x_min) scr_dirty.x_min  = (x0); \
		if ((int)(y1) > scr_dirty.y_max) scr_dirty.y_max  = (y1); \
		if ((int)(x1) > scr_dirty.x_max) scr_dirty.x_max  = (x1); \
	} while(0)

#define NOTIFY_LOCK() do { \
							notifyLock++; \
						} while(0)
            
#define NOTIFY_DONE(updateTopics) do { \
							lockTopics |= (updateTopics); \
							if (notifyLock > 0) notifyLock--; \
							if (notifyLock == 0) { \
								screen_notifyChange(lockTopics); \
								lockTopics = 0;\
							} \
						} while(0)

/** Clear the hanging attribute if the cursor is no longer >= W */
#define clear_invalid_hanging() do { \
									if (cursor.hanging && cursor.x != W-1) { \
										cursor.hanging = false; \
										screen_notifyChange(TOPIC_CHANGE_CURSOR); \
									} \
								} while(false)

#define cursor_inside_region() (cursor.y >= TOP && cursor.y <= BTM)

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
	termconf->show_buttons = SCR_DEF_SHOW_BUTTONS;
	termconf->show_config_links = SCR_DEF_SHOW_MENU;
	termconf->cursor_shape = SCR_DEF_CURSOR_SHAPE;
	termconf->crlf_mode = SCR_DEF_CRLF;
	termconf->want_all_fn = SCR_DEF_ALLFN;
	termconf->debugbar = SCR_DEF_DEBUGBAR;
	termconf->allow_decopt_12 = SCR_DEF_DECOPT12;
	termconf->ascii_debug = SCR_DEF_ASCIIDEBUG;
	termconf->backdrop[0] = 0;
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

	// Migrate
	if (termconf->config_version < 1) {
		persist_dbg("termconf: Updating to version %d", 1);
		termconf->debugbar = SCR_DEF_DEBUGBAR;
		changed = 1;
	}
	if (termconf->config_version < 2) {
		persist_dbg("termconf: Updating to version %d", 1);
		termconf->allow_decopt_12 = SCR_DEF_DECOPT12;
		changed = 1;
	}
	if (termconf->config_version < 3) {
		persist_dbg("termconf: Updating to version %d", 1);
		termconf->ascii_debug = SCR_DEF_ASCIIDEBUG;
		changed = 1;
	}
	if (termconf->config_version < 4) {
		persist_dbg("termconf: Updating to version %d", 1);
		termconf->backdrop[0] = 0;
		changed = 1;
	}

	termconf->config_version = TERMCONF_VERSION;

	// Validation...
	if (termconf->display_cooldown_ms == 0) {
		termconf->display_cooldown_ms = 1;
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

	reset_screen_dirty();
	screen_reset();
}

/**
 * Reset the cursor position & colors
 */
static void ICACHE_FLASH_ATTR
cursor_reset(void)
{
	NOTIFY_LOCK();

	cursor.x = 0;
	cursor.y = 0;
	cursor.hanging = false;

	cursor.charsetN = 0;
	cursor.charset0 = CS_B_USASCII;
	cursor.charset1 = CS_0_DEC_SUPPLEMENTAL;

	screen_reset_sgr();

	NOTIFY_DONE(TOPIC_CHANGE_CURSOR);
}

/**
 * Reset the screen - called by ESC c
 */
static void ICACHE_FLASH_ATTR
screen_reset_on_resize(void)
{
	ansi_dbg("Screen partial reset due to resize");
	NOTIFY_LOCK();

	cursor_reset();

	scr.vm0 = 0;
	scr.vm1 = H-1;

	// size is left unchanged
	screen_clear(CLEAR_ALL); // also clears utf cache

	NOTIFY_DONE(TOPIC_INTERNAL);
}

/**
 * Reset the cursor (\e[0m)
 */
void ICACHE_FLASH_ATTR
screen_reset_sgr(void)
{
	NOTIFY_LOCK();

	cursor.fg = 0;
	cursor.bg = 0;
	cursor.attrs = 0;
	cursor.conceal = false;

	NOTIFY_DONE(TOPIC_INTERNAL);
}

static void ICACHE_FLASH_ATTR
screen_reset_do(bool size, bool labels)
{
	ScreenNotifyTopics topics = TOPIC_CHANGE_SCREEN_OPTS | TOPIC_CHANGE_CURSOR | TOPIC_CHANGE_CONTENT_ALL;
	NOTIFY_LOCK();

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

	state_backup.alternate_active = false;

	mouse_tracking.encoding = MTE_SIMPLE;
	mouse_tracking.focus_tracking = false;
	mouse_tracking.mode = MTM_NONE;

	if (size) {
		W = termconf->width;
		H = termconf->height;
	}

	scr.vm0 = 0;
	scr.vm1 = H - 1;
	cursor_reset();
	screen_clear(CLEAR_ALL); // also clears utf cache

	// Set initial tabstops
	for (int i = 0; i < TABSTOP_WORDS; i++) {
		scr.tab_stops[i] = 0x80808080;
	}

	if (labels) {
		strcpy(termconf_live.title, termconf->title);
		strcpy(termconf_live.backdrop, termconf->backdrop);

		for (int i = 1; i <= TERM_BTN_COUNT; i++) {
			strcpy(termconf_live.btn[i], termconf->btn[i]);
			strcpy(termconf_live.btn_msg[i], termconf->btn_msg[i]);
		}

		termconf_live.show_buttons = termconf->show_buttons;
		termconf_live.show_config_links = termconf->show_config_links;

		topics |= TOPIC_CHANGE_TITLE | TOPIC_CHANGE_BUTTONS | TOPIC_CHANGE_BACKDROP;
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

	NOTIFY_DONE(topics);
}

/**
 * Reset the screen - called by ESC c
 */
void ICACHE_FLASH_ATTR
screen_reset(void)
{
	ansi_dbg("Screen reset.");
	screen_reset_do(true, true);
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

	NOTIFY_LOCK();

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
		memcpy(termconf_live.title, state_backup.title, TERM_TITLE_LEN);
		memcpy(termconf_live.btn, state_backup.btn, sizeof(termconf_live.btn));
		memcpy(termconf_live.btn_msg, state_backup.btn_msg, sizeof(termconf_live.btn_msg));
		memcpy(scr.tab_stops, state_backup.tab_stops, sizeof(scr.tab_stops));
		scr.vm0 = state_backup.vm0;
		scr.vm1 = state_backup.vm1;
		// this may clear the screen as a side effect if size changed
		screen_resize(state_backup.height, state_backup.width);
		// TODO restore screen content (if this is ever possible)
	}

	state_backup.alternate_active = alternate;
	NOTIFY_DONE(TOPIC_INITIAL);
}

//endregion

//region --- Tab stops ---

void ICACHE_FLASH_ATTR
screen_clear_all_tabs(void)
{
	NOTIFY_LOCK();
	memset(scr.tab_stops, 0, sizeof(scr.tab_stops));
	NOTIFY_DONE(TOPIC_INTERNAL);
}

void ICACHE_FLASH_ATTR
screen_set_tab(void)
{
	NOTIFY_LOCK();
	scr.tab_stops[cursor.x/32] |= (1<<(cursor.x%32));
	NOTIFY_DONE(TOPIC_INTERNAL);
}

void ICACHE_FLASH_ATTR
screen_clear_tab(void)
{
	NOTIFY_LOCK();
	scr.tab_stops[cursor.x/32] &= ~(1<<(cursor.x%32));
	NOTIFY_DONE(TOPIC_INTERNAL);
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
	NOTIFY_DONE(TOPIC_CHANGE_CURSOR);
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
	NOTIFY_DONE(TOPIC_CHANGE_CURSOR);
}

//endregion

//region --- Clearing & inserting ---

/**
 * Clear range, inclusive
 *
 * @param from - starting absolute position
 * @param to - ending absolute position
 * @param clear_utf - release any encountered utf8
 */
static void ICACHE_FLASH_ATTR
clear_range_do(unsigned int from, unsigned int to, bool clear_utf)
{
	if (to >= W*H) to = W*H-1;

	Cell sample;
	sample.symbol = ' ';
	sample.fg = cursor.fg;
	sample.bg = cursor.bg;
	// we discard all attributes except color-set flags
	sample.attrs = (CellAttrs) (cursor.attrs & (ATTR_FG | ATTR_BG));

	// if no colors, always use 0,0
	if (0 == sample.attrs) {
		sample.fg = sample.bg = 0;
	}

	for (unsigned int i = from; i <= to; i++) {
		if (clear_utf) {
			UnicodeCacheRef symbol = screen[i].symbol;
			if (IS_UNICODE_CACHE_REF(symbol)) unicode_cache_remove(symbol);
		}
		memcpy(&screen[i], &sample, sizeof(Cell));
	}
}

/**
 * Clear range, inclusive, freeing any encountered UTF8 from the cache
 *
 * @param from - starting absolute position
 * @param to - ending absolute position
 */
static inline void ICACHE_FLASH_ATTR
clear_range_utf(unsigned int from, unsigned int to)
{
	clear_range_do(from, to, true);
}

/**
 * Clear range, inclusive. Do not release utf characters
 *
 * @param from - starting absolute position
 * @param to - ending absolute position
 */
static inline void ICACHE_FLASH_ATTR
clear_range_noutf(unsigned int from, unsigned int to)
{
	clear_range_do(from, to, false);
}

/**
 * Free a utf8 reference character in a cell
 *
 * @param row
 * @param col
 */
static inline void ICACHE_FLASH_ATTR
utf_free_cell(int row, int col)
{
	//dbg("free cell (row %d) %d", row, col);
	UnicodeCacheRef symbol = screen[row * W + col].symbol;
	if (IS_UNICODE_CACHE_REF(symbol))
		unicode_cache_remove(symbol);
}

/**
 * Back-up utf8 reference in a cell (i.e. increment the counter,
 * so 1 subsequent free has no effect)
 *
 * @param row
 * @param col
 */
static inline void ICACHE_FLASH_ATTR
utf_backup_cell(int row, int col)
{
	//dbg("backup cell (row %d) %d", row, col);
	UnicodeCacheRef symbol = screen[row * W + col].symbol;
	if (IS_UNICODE_CACHE_REF(symbol))
		unicode_cache_inc(symbol);
}

/**
 * Duplicate a cell within a row
 * @param row - row to work on
 * @param dest_col - destination column
 * @param src_col - source column
 */
static inline void ICACHE_FLASH_ATTR
copy_cell(int row, int dest_col, int src_col)
{
	//dbg("copy cell (row %d) %d -> %d", row, src_col, dest_col);
	memcpy(screen+row*W+dest_col, screen+row*W+src_col, sizeof(Cell));
}

/**
 * Free all utf8 on a row
 *
 * @param row
 */
static inline void ICACHE_FLASH_ATTR
utf_free_row(int row)
{
	//dbg("free row %d", row);
	for (int col = 0; col < W; col++) {
		utf_free_cell(row, col);
	}
}

/**
 * Back-up all utf8 refs on a row
 *
 * @param row
 */
static inline void ICACHE_FLASH_ATTR
utf_backup_row(int row)
{
	//dbg("backup row %d", row);
	for (int col = 0; col < W; col++) {
		utf_backup_cell(row, col);
	}
}

/**
 * Duplicate a row
 *
 * @param dest - destination row number (0-based)
 * @param src  - source row number (0-based)
 */
static inline void ICACHE_FLASH_ATTR
copy_row(int dest, int src)
{
	//dbg("copy row %d -> %d", src, dest);
	memcpy(screen + dest * W, screen + src * W, sizeof(Cell) * W);
}

/**
 * Clear a row, do nothing with the utf8 cache
 *
 * @param row
 */
static inline void ICACHE_FLASH_ATTR
clear_row_noutf(int row)
{
	clear_range_noutf(row * W, (row + 1) * W - 1);
}

/**
 * Clear a row, freeing any utf8 refs
 *
 * @param row
 */
static inline void ICACHE_FLASH_ATTR
clear_row_utf(int row)
{
	clear_range_utf(row * W, (row + 1) * W - 1);
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
			unicode_cache_clear();
			clear_range_noutf(0, W * H - 1);
			scr.last_char[0]  = 0;
			break;

		case CLEAR_FROM_CURSOR:
			clear_range_utf((cursor.y * W) + cursor.x, W * H - 1);
			expand_dirty(cursor.y, H-1, 0, W-1);
			break;

		case CLEAR_TO_CURSOR:
			clear_range_utf(0, (cursor.y * W) + cursor.x);
			expand_dirty(0, cursor.y, 0, W-1);
			break;
	}
	NOTIFY_DONE(mode == CLEAR_ALL ? TOPIC_CHANGE_CONTENT_ALL : TOPIC_CHANGE_CONTENT_PART);
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
			clear_row_utf(cursor.y);
			expand_dirty(cursor.y, cursor.y, 0, W-1);
			break;

		case CLEAR_FROM_CURSOR:
			clear_range_utf(cursor.y * W + cursor.x, (cursor.y + 1) * W - 1);
			expand_dirty(cursor.y, cursor.y, cursor.x, W-1);
			break;

		case CLEAR_TO_CURSOR:
			clear_range_utf(cursor.y * W, cursor.y * W + cursor.x);
			expand_dirty(cursor.y, cursor.y, 0, cursor.x);
			break;
	}
	NOTIFY_DONE(TOPIC_CHANGE_CONTENT_PART);
}

void ICACHE_FLASH_ATTR
screen_clear_in_line(unsigned int count)
{
	NOTIFY_LOCK();
	if (cursor.x + count >= W) {
		screen_clear_line(CLEAR_FROM_CURSOR);
	} else {
		clear_range_utf(cursor.y * W + cursor.x, cursor.y * W + cursor.x + count - 1);
		expand_dirty(cursor.y, cursor.y, cursor.x, cursor.x + count - 1);
	}
	NOTIFY_DONE(TOPIC_CHANGE_CONTENT_PART);
}

void ICACHE_FLASH_ATTR
screen_insert_lines(unsigned int lines)
{
	if (!cursor_inside_region()) return; // can't insert if not inside region
	NOTIFY_LOCK();

	// shift the following lines
	int targetStart = cursor.y + lines;
	if (targetStart > BTM) {
		clear_range_utf(cursor.y*W, (BTM+1)*W-1);
	} else {
		// do the moving
		for (int i = BTM; i >= targetStart; i--) {
			utf_free_row(i); // release old characters
			copy_row(i, i - lines);
			if (i != targetStart) utf_backup_row(i);
		}

		// clear the first line
		clear_row_noutf(cursor.y);
		// copy it to the rest of the cleared region
		for (int i = cursor.y+1; i < targetStart; i++) {
			copy_row(i, cursor.y);
		}
	}
	expand_dirty(cursor.y, BTM, 0, W - 1);
	NOTIFY_DONE(TOPIC_CHANGE_CONTENT_PART);
}

void ICACHE_FLASH_ATTR
screen_delete_lines(unsigned int lines)
{
	if (!cursor_inside_region()) return; // can't delete if not inside region
	NOTIFY_LOCK();

	// shift lines up
	int movedBlockEnd = BTM - lines ;
	if (movedBlockEnd <= cursor.y) {
		// clear the entire rest of the screen
		movedBlockEnd = cursor.y;
		clear_range_utf(movedBlockEnd*W, (BTM+1)*W-1);
	} else {
		// move some lines up, clear the rest
		for (int i = cursor.y; i <= movedBlockEnd; i++) {
			utf_free_row(i);
			copy_row(i, i+lines);
			if (i != movedBlockEnd) utf_backup_row(i);
		}
		clear_range_noutf((movedBlockEnd+1)*W, (BTM+1)*W-1);
	}

	expand_dirty(cursor.y, BTM, 0, W - 1);
	NOTIFY_DONE(TOPIC_CHANGE_CONTENT_PART);
}

void ICACHE_FLASH_ATTR
screen_insert_characters(unsigned int count)
{
	NOTIFY_LOCK();

	// shove rest of the line to the right

	int targetStart = cursor.x + count;
	if (targetStart >= W) {
		// all rest of line was cleared
		clear_range_utf(cursor.y * W + cursor.x, (cursor.y + 1) * W - 1);
	} else {
		// do the moving
		for (int i = W-1; i >= targetStart; i--) {
			utf_free_cell(cursor.y, i);
			copy_cell(cursor.y, i, i - count);
			utf_backup_cell(cursor.y, i);
		}
		clear_range_utf(cursor.y * W + cursor.x, cursor.y * W + targetStart - 1);
	}
	expand_dirty(cursor.y, cursor.y, cursor.x, W - 1);
	NOTIFY_DONE(TOPIC_CHANGE_CONTENT_PART);
}

void ICACHE_FLASH_ATTR
screen_delete_characters(unsigned int count)
{
	NOTIFY_LOCK();

	// pull rest of the line to the left

	int movedBlockEnd = W - count;
	if (movedBlockEnd > cursor.x) {
		// partial line delete / move

		for (int i = cursor.x; i <= movedBlockEnd; i++) {
			utf_free_cell(cursor.y, i);
			copy_cell(cursor.y, i, i + count);
			utf_backup_cell(cursor.y, i);
		}
		// clear original positions of the moved characters
		clear_range_noutf(cursor.y * W + (W - count), (cursor.y + 1) * W - 1);
	} else {
		// all rest was cleared
		screen_clear_line(CLEAR_FROM_CURSOR);
	}

	expand_dirty(cursor.y, cursor.y, cursor.x, W - 1);
	NOTIFY_DONE(TOPIC_CHANGE_CONTENT_PART);
}
//endregion

//region --- Entire screen manipulation ---

void ICACHE_FLASH_ATTR
screen_fill_with_E(void)
{
	NOTIFY_LOCK();
	screen_reset_do(false, false); // based on observation from xterm

	Cell sample;
	sample.symbol = 'E';
	sample.fg = 0;
	sample.bg = 0;
	sample.attrs = 0;

	for (unsigned int i = 0; i <= W*H-1; i++) {
		memcpy(&screen[i], &sample, sizeof(Cell));
	}
	NOTIFY_DONE(TOPIC_CHANGE_CONTENT_ALL);
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
	NOTIFY_DONE(TOPIC_CHANGE_SCREEN_OPTS|TOPIC_CHANGE_CONTENT_ALL|TOPIC_CHANGE_CURSOR);
}

void ICACHE_FLASH_ATTR
screen_set_title(const char *title)
{
	NOTIFY_LOCK();
	strncpy(termconf_live.title, title, TERM_TITLE_LEN);
	NOTIFY_DONE(TOPIC_CHANGE_TITLE);
}

/**
 * Helper function to set terminal button label
 * @param num - button number 1-5
 * @param str - button text
 */
void ICACHE_FLASH_ATTR
screen_set_button_text(int num, const char *text)
{
	NOTIFY_LOCK();
	strncpy(termconf_live.btn[num-1], text, TERM_BTN_LEN);
	NOTIFY_DONE(TOPIC_CHANGE_BUTTONS);
}

/**
 * Helper function to set terminalbackdrop
 * @param url - url
 */
void ICACHE_FLASH_ATTR
screen_set_backdrop(const char *url)
{
	NOTIFY_LOCK();
	strncpy(termconf_live.backdrop, url, TERM_BACKDROP_LEN);
	NOTIFY_DONE(TOPIC_CHANGE_BACKDROP);
}

/**
 * Shift screen upwards
 */
void ICACHE_FLASH_ATTR
screen_scroll_up(unsigned int lines)
{
	NOTIFY_LOCK();
	if (lines >= RH) {
		// clear entire region
		clear_range_utf(TOP * W, (BTM + 1) * W - 1);
		goto done;
	}

	// bad cmd
	if (lines == 0) {
		goto done;
	}

	int y;
	for (y = TOP; y <= BTM - lines; y++) {
		utf_free_row(y);
		copy_row(y, y+lines);
		if (y < BTM - lines) utf_backup_row(y);
	}

	clear_range_noutf(y * W, (BTM + 1) * W - 1);

done:
	expand_dirty(TOP, BTM, 0, W - 1);
	NOTIFY_DONE(TOPIC_CHANGE_CONTENT_PART);
}

/**
 * Shift screen downwards
 */
void ICACHE_FLASH_ATTR
screen_scroll_down(unsigned int lines)
{
	NOTIFY_LOCK();
	if (lines >= RH) {
		// clear entire region
		clear_range_utf(TOP * W, (BTM + 1) * W - 1);
		goto done;
	}

	// bad cmd
	if (lines == 0) {
		goto done;
	}

	int y;
	for (y = BTM; y >= TOP+lines; y--) {
		utf_free_row(y);
		copy_row(y, y-lines);
		if (y > TOP + lines) utf_backup_row(y);
	}

	clear_range_noutf(TOP * W, TOP * W + lines * W - 1);
done:
	expand_dirty(TOP, BTM, 0, W - 1);
	NOTIFY_DONE(TOPIC_CHANGE_CONTENT_PART);
}

/** Set scrolling region */
void ICACHE_FLASH_ATTR
screen_set_scrolling_region(int from, int to)
{
	NOTIFY_LOCK();
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
	NOTIFY_DONE(TOPIC_INTERNAL);
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
	NOTIFY_DONE(TOPIC_CHANGE_SCREEN_OPTS);
}

/** set cursor blink option */
void ICACHE_FLASH_ATTR
screen_cursor_blink(bool blink)
{
	if (!termconf->allow_decopt_12) return;

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
	NOTIFY_DONE(TOPIC_CHANGE_SCREEN_OPTS);
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
	NOTIFY_DONE(TOPIC_CHANGE_CURSOR);
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
		*y -= TOP;
	}
}

/* Report scrolling region */
void ICACHE_FLASH_ATTR
screen_region_get(int *pv0, int *pv1)
{
	*pv0 = TOP;
	*pv1 = BTM;
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
	NOTIFY_DONE(TOPIC_CHANGE_CURSOR);
}

/**
 * Set cursor Y position
 */
void ICACHE_FLASH_ATTR
screen_cursor_set_y(int y)
{
	NOTIFY_LOCK();
	if (cursor.origin_mode) {
		y += TOP;
		if (y > BTM) y = BTM;
		if (y < TOP) y = TOP;
	} else {
		if (y > H-1) y = H-1;
		if (y < 0) y = 0;
	}
	cursor.y = y;
	NOTIFY_DONE(TOPIC_CHANGE_CURSOR);
}

/**
 * Relative cursor move
 */
void ICACHE_FLASH_ATTR
screen_cursor_move(int dy, int dx, bool scroll)
{
	NOTIFY_LOCK();
	int move;
	bool scrolled = 0;

	clear_invalid_hanging();

	if (cursor.hanging && dx < 0) {
		//dx += 1; // consume one step on the removal of "xenl"
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
						cursor.y = BTM;
						cursor.x = W - 1;
					}
				}
			}
		} else {
			cursor.x = 0;
		}
	}

	if (cursor.y < TOP) {
		if (was_inside) {
			move = -(cursor.y - TOP);
			cursor.y = TOP;
			if (scroll) {
				screen_scroll_down((unsigned int) move);
				scrolled = true;
			}
		}
		else {
			// outside the region, just validate that we're not going offscreen
			// scrolling is not possible in this case
			if (cursor.y < 0) {
				cursor.y = 0;
			}
		}
	}

	if (cursor.y > BTM) {
		if (was_inside) {
			move = cursor.y - BTM;
			cursor.y = BTM;
			if (scroll) {
				screen_scroll_up((unsigned int) move);
				scrolled = true;
			}
		}
		else {
			// outside the region, just validate that we're not going offscreen
			// scrolling is not possible in this case
			if (cursor.y >= H) {
				cursor.y = H-1;
			}
		}
	}

	if (scrolled) {
		expand_dirty(TOP, BTM, 0, W-1);
	}

	NOTIFY_DONE(TOPIC_CHANGE_CURSOR | (scrolled*TOPIC_CHANGE_CONTENT_PART));
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

	NOTIFY_DONE(TOPIC_CHANGE_CURSOR);
}

void ICACHE_FLASH_ATTR
screen_back_index(int count)
{
	NOTIFY_LOCK();
	ScreenNotifyTopics topics = TOPIC_CHANGE_CURSOR;
	int new_x = cursor.x - count;
	if (new_x >= 0) {
		cursor.x = new_x;
	} else {
		cursor.x = 0;
		screen_insert_characters(-new_x);
		topics |= TOPIC_CHANGE_CONTENT_PART;
	}
	NOTIFY_DONE(topics);
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
	NOTIFY_DONE(TOPIC_CHANGE_SCREEN_OPTS);
}

/**
 * Enable autowrap
 */
void ICACHE_FLASH_ATTR
screen_wrap_enable(bool enable)
{
	NOTIFY_LOCK();
	cursor.auto_wrap = enable;
	NOTIFY_DONE(TOPIC_INTERNAL);
}

/**
 * Enable reverse wraparound
 */
void ICACHE_FLASH_ATTR
screen_reverse_wrap_enable(bool enable)
{
	NOTIFY_LOCK();
	cursor.reverse_wrap = enable;
	NOTIFY_DONE(TOPIC_INTERNAL);
}

/**
 * Set cursor foreground color
 */
void ICACHE_FLASH_ATTR
screen_set_fg(Color color)
{
	NOTIFY_LOCK();
	cursor.fg = color;
	cursor.attrs |= ATTR_FG;
	NOTIFY_DONE(TOPIC_INTERNAL);
}

/**
 * Set cursor background coloor
 */
void ICACHE_FLASH_ATTR
screen_set_bg(Color color)
{
	NOTIFY_LOCK();
	cursor.bg = color;
	cursor.attrs |= ATTR_BG;
	NOTIFY_DONE(TOPIC_INTERNAL);
}

/**
 * Set cursor foreground color to default
 */
void ICACHE_FLASH_ATTR
screen_set_default_fg(void)
{
	NOTIFY_LOCK();
	cursor.fg = 0;
	cursor.attrs &= ~ATTR_FG;
	NOTIFY_DONE(TOPIC_INTERNAL);
}

/**
 * Set cursor background color to default
 */
void ICACHE_FLASH_ATTR
screen_set_default_bg(void)
{
	NOTIFY_LOCK();
	cursor.bg = 0;
	cursor.attrs &= ~ATTR_BG;
	NOTIFY_DONE(TOPIC_INTERNAL);
}

void ICACHE_FLASH_ATTR
screen_set_sgr(CellAttrs attrs, bool ena)
{
	NOTIFY_LOCK();
	if (ena) {
		cursor.attrs |= attrs;
	}
	else {
		cursor.attrs &= ~attrs;
	}
	NOTIFY_DONE(TOPIC_INTERNAL);
}

void ICACHE_FLASH_ATTR
screen_set_sgr_conceal(bool ena)
{
	NOTIFY_LOCK();
	cursor.conceal = ena;
	NOTIFY_DONE(TOPIC_INTERNAL);
}

void ICACHE_FLASH_ATTR
screen_set_charset_n(int Gx)
{
	NOTIFY_LOCK();
	if (Gx < 0 || Gx > 1) return; // bad n
	cursor.charsetN = Gx;
	NOTIFY_DONE(TOPIC_INTERNAL);
}

void ICACHE_FLASH_ATTR
screen_set_charset(int Gx, char charset)
{
	NOTIFY_LOCK();
	if (Gx == 0) cursor.charset0 = charset;
	else if (Gx == 1) cursor.charset1 = charset;
	NOTIFY_DONE(TOPIC_INTERNAL);
}

void ICACHE_FLASH_ATTR
screen_set_insert_mode(bool insert)
{
	NOTIFY_LOCK();
	scr.insert_mode = insert;
	NOTIFY_DONE(TOPIC_INTERNAL);
}

void ICACHE_FLASH_ATTR
screen_set_numpad_alt_mode(bool alt_mode)
{
	NOTIFY_LOCK();
	scr.numpad_alt_mode = alt_mode;
	NOTIFY_DONE(TOPIC_CHANGE_SCREEN_OPTS);
}

void ICACHE_FLASH_ATTR
screen_set_cursors_alt_mode(bool alt_mode)
{
	NOTIFY_LOCK();
	scr.cursors_alt_mode = alt_mode;
	NOTIFY_DONE(TOPIC_CHANGE_SCREEN_OPTS);
}

void ICACHE_FLASH_ATTR
screen_set_reverse_video(bool reverse)
{
	NOTIFY_LOCK();
	scr.reverse_video = reverse;
	NOTIFY_DONE(TOPIC_CHANGE_SCREEN_OPTS);
}

void ICACHE_FLASH_ATTR
screen_set_bracketed_paste(bool ena)
{
	NOTIFY_LOCK();
	scr.bracketed_paste = ena;
	NOTIFY_DONE(TOPIC_CHANGE_SCREEN_OPTS);
}

void ICACHE_FLASH_ATTR
screen_set_newline_mode(bool nlm)
{
	NOTIFY_LOCK();
	termconf_live.crlf_mode = nlm;
	NOTIFY_DONE(TOPIC_CHANGE_SCREEN_OPTS);
}

void ICACHE_FLASH_ATTR
screen_set_origin_mode(bool region_origin)
{
	NOTIFY_LOCK();
	cursor.origin_mode = region_origin;
	screen_cursor_set(0, 0);
	NOTIFY_DONE(TOPIC_INTERNAL);
}

static void ICACHE_FLASH_ATTR
do_save_private_opt(int n, bool save)
{
	ScreenNotifyTopics topics = TOPIC_INTERNAL;
	if (!save) NOTIFY_LOCK();
#define SAVE_RESTORE(sf, of) do { if (save) sf=(of); else of=(sf); } while(0)
	switch (n) {
		case 1:
			SAVE_RESTORE(opt_backup.cursors_alt_mode, scr.cursors_alt_mode);
			topics |= TOPIC_CHANGE_SCREEN_OPTS;
			break;
		case 5:
			SAVE_RESTORE(opt_backup.reverse_video, scr.reverse_video);
			break;
		case 6:
			SAVE_RESTORE(opt_backup.origin_mode, cursor.origin_mode); // XXX maybe we should move cursor to 1,1 if it's restored to True
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
			topics |= TOPIC_CHANGE_SCREEN_OPTS;
			break;
		case 1004:
			SAVE_RESTORE(opt_backup.focus_tracking, mouse_tracking.focus_tracking);
			topics |= TOPIC_CHANGE_SCREEN_OPTS;
			break;
		case 1005:
		case 1006:
		case 1015:
			SAVE_RESTORE(opt_backup.mouse_encoding, mouse_tracking.encoding);
			topics |= TOPIC_CHANGE_SCREEN_OPTS;
			break;
		case 12: // cursor blink
			if (save) {
				opt_backup.cursor_blink = CURSOR_BLINKS(termconf_live.cursor_shape);
			} else {
				screen_cursor_blink(opt_backup.cursor_blink);
			}
			topics |= TOPIC_CHANGE_SCREEN_OPTS;
			break;
		case 25:
			SAVE_RESTORE(opt_backup.cursor_visible, scr.cursor_visible);
			topics |= TOPIC_CHANGE_SCREEN_OPTS;
			break;
		case 45:
			SAVE_RESTORE(opt_backup.reverse_wrap, cursor.reverse_wrap);
			break;
		case 2004:
			SAVE_RESTORE(opt_backup.bracketed_paste, scr.bracketed_paste);
			break;
		case 800:
			SAVE_RESTORE(opt_backup.show_buttons, termconf_live.show_buttons);
			topics |= TOPIC_CHANGE_SCREEN_OPTS;
			break;
		case 801:
			SAVE_RESTORE(opt_backup.show_config_links, termconf_live.show_config_links);
			topics |= TOPIC_CHANGE_SCREEN_OPTS;
			break;
		default:
			ansi_warn("Cannot store ?%d", n);
	}
	if (!save) NOTIFY_DONE(topics);
}

void ICACHE_FLASH_ATTR
screen_save_private_opt(int n)
{
	NOTIFY_LOCK();
	do_save_private_opt(n, true);
	NOTIFY_DONE(TOPIC_INTERNAL);
}

void ICACHE_FLASH_ATTR
screen_restore_private_opt(int n)
{
	NOTIFY_LOCK();
	do_save_private_opt(n, false);
	NOTIFY_DONE(TOPIC_INTERNAL);
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
	if (cursor.attrs & ATTR_INVERSE) buffer += sprintf(buffer, ";%d", SGR_INVERSE);
	if (cursor.attrs & ATTR_FG)
		buffer += sprintf(buffer, ";%d", ((cursor.fg > 7) ? SGR_FG_BRT_START : SGR_FG_START) + (cursor.fg&7));
	if (cursor.attrs & ATTR_BG)
		buffer += sprintf(buffer, ";%d", ((cursor.bg > 7) ? SGR_BG_BRT_START : SGR_BG_START) + (cursor.bg&7));
	(void)buffer;
}

//endregion

//region --- Printing ---

static const char* ICACHE_FLASH_ATTR
putchar_graphic(const char *ch)
{
	static char buf[4];

	NOTIFY_LOCK();
	ScreenNotifyTopics topics = TOPIC_CHANGE_CURSOR;

	if (cursor.hanging) {
		// perform the scheduled wrap if hanging
		// if auto-wrap = off, it overwrites the last char
		if (cursor.auto_wrap) {
			cursor.x = 0;
			cursor.y++;
			// Y wrap
			if (cursor.y > BTM) {
				// Scroll up, so we have space for writing
				screen_scroll_up(1);
				cursor.y = BTM;
			}

			cursor.hanging = false;
		}
	}

	Cell *c = &screen[cursor.x + cursor.y * W];

	// move the rest of the line if we're in Insert Mode
	if (cursor.x < W-1 && scr.insert_mode) screen_insert_characters(1);

	char chs = (cursor.charsetN == 0) ? cursor.charset0 : cursor.charset1;
	if (chs != 'B' && ch[1] == 0 && ch[0] <= 0x7f) {
		// we have len=1 and ASCII, can be re-mapped using a table
		utf8_remap(buf, ch[0], chs);
		ch = buf;
	}

	UnicodeCacheRef oldSymbol = c->symbol;
	Color oldFg = c->fg;
	Color oldBg = c->bg;
	CellAttrs oldAttrs = c->attrs;

	unicode_cache_remove(c->symbol);
	c->symbol = unicode_cache_add((const u8 *)ch);
	c->fg = cursor.fg;
	c->bg = cursor.bg;
	c->attrs = cursor.attrs;

	if (c->symbol != oldSymbol || c->fg != oldFg || c->bg != oldBg || c->attrs != oldAttrs) {
		expand_dirty(cursor.y, cursor.y, cursor.x, cursor.x);
		topics |= TOPIC_CHANGE_CONTENT_PART;
	}

	cursor.x++;
	// X wrap
	if (cursor.x >= W) {
		cursor.hanging = true; // hanging - next typed char wraps around, but backspace and arrows still stay on the same line.
		cursor.x = W - 1;
	}

	NOTIFY_DONE(topics);
	return ch;
}

/**
 * Set a character in the cursor color, move to right with wrap.
 */
void ICACHE_FLASH_ATTR
screen_putchar(const char *ch)
{
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
	return;
}

/**
 * Repeat last graphic character
 * @param count
 */
void ICACHE_FLASH_ATTR
screen_repeat_last_character(int count)
{
	if (scr.last_char[0]==0) {
		scr.last_char[0] = ' ';
		scr.last_char[1] = 0;
	}

	if (count > W*H) count = W*H;

	while (count > 0) {
		putchar_graphic(scr.last_char);
		count--;
	}
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

		case CS_2_BLOCKS_LINES: /* ESPTerm Character Rom 2 */
			if ((g >= CODEPAGE_2_BEGIN) && (g <= CODEPAGE_2_END)) {
				n = codepage_2[g - CODEPAGE_2_BEGIN];
				if (n) utf = n;
			}
			break;

		case CS_3_LINES_EXTRA: /* ESPTerm Character Rom 3 */
			if ((g >= CODEPAGE_3_BEGIN) && (g <= CODEPAGE_3_END)) {
				n = codepage_3[g - CODEPAGE_3_BEGIN];
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

	utf8_encode(out, utf, false);
}
//endregion

//region --- Serialization ---

struct ScreenSerializeState {
	Color lastFg;
	Color lastBg;
	Color lastLiveFg;
	Color lastLiveBg;
	CellAttrs lastAttrs;
	UnicodeCacheRef lastSymbol;
	char lastChar[4];
	u8 lastCharLen;
	int index; // index in the screen buffer
	ScreenNotifyTopics topics;
	ScreenNotifyTopics last_topic;
	ScreenNotifyTopics current_topic;
	bool partial;
	int x_min, x_max, y_min, y_max;
	int i_max;
	int i_start;
	bool first;
};

/**
 * Serialize the screen to a data buffer. May need multiple calls if the buffer is insufficient in size.
 *
 * @warning MAKE SURE *DATA IS NULL BEFORE FIRST CALL!
 *          Call with NULL 'buffer' at the end to free the data struct.
 *
 * @param buffer - buffer array of limited size. If NULL, indicates this is the last call.
 * @param buf_len - buffer array size
 * @param topics - what should be included in the message (ignored after the first call)
 * @param data - opaque pointer to internal data structure for storing state between repeated calls
 *               if NULL, indicates this is the first call; the structure will be allocated.
 *
 * @return HTTPD_CGI_DONE or HTTPD_CGI_MORE. If more, repeat with the same `data` pointer.
 */
httpd_cgi_state ICACHE_FLASH_ATTR
screenSerializeToBuffer(char *buffer, size_t buf_len, ScreenNotifyTopics topics, void **data)
{
	struct ScreenSerializeState *ss = *data;

	if (buffer == NULL) {
		if (ss != NULL) free(ss);
		return HTTPD_CGI_DONE;
	}

	Cell *cell, *cell0;      // temporary cell pointers for finding repetitions

	u8 nbytes;               // temporary variable for utf writing utilities
	size_t remain = buf_len; // remaining space in the output buffer
	char *bb = buffer;       // write pointer

#define bufput_c(c) do { \
			*bb = (char)(c); \
			bb++; \
			remain--; \
		} while(0)

#define bufput_utf8(num) do { \
		nbytes = utf8_encode(bb, (num)+1, true); \
		bb += nbytes; \
		remain -= nbytes; \
	} while(0)

#define bufput_t_utf8(t, num) do { \
		bufput_c((t)); \
		bufput_utf8((num)); \
	} while(0)

	// tags for screen serialization
#define SEQ_TAG_SKIP '\x01'
#define SEQ_TAG_REPEAT '\x02'
#define SEQ_TAG_COLORS '\x03'
#define SEQ_TAG_ATTRS '\x04'
#define SEQ_TAG_FG '\x05'
#define SEQ_TAG_BG '\x06'
#define SEQ_TAG_ATTRS_0 '\x07'

#define TOPICMARK_SCREEN_OPTS 'O'
#define TOPICMARK_TITLE   'T'
#define TOPICMARK_BUTTONS 'B'
#define TOPICMARK_DEBUG   'D'
#define TOPICMARK_BELL    '!'
#define TOPICMARK_CURSOR  'C'
#define TOPICMARK_SCREEN   'S'
#define TOPICMARK_BACKDROP 'W'

	if (ss == NULL) {
		// START!

		*data = ss = malloc(sizeof(struct ScreenSerializeState));

		if (topics == 0 || termconf_live.debugbar) {
			topics |= TOPIC_INTERNAL;
		}

		if (topics & TOPIC_CHANGE_CONTENT_PART) {
			// reset dirty extents
			ss->partial = true;

			ss->x_min = scr_dirty.x_min;
			ss->x_max = scr_dirty.x_max;
			ss->y_min = scr_dirty.y_min;
			ss->y_max = scr_dirty.y_max;

			if (ss->x_min > ss->x_max || ss->y_min > ss->y_max) {
				seri_warn("Partial redraw, but bad bounds! X %d..%d, Y %d..%d", ss->x_min, ss->x_max, ss->y_min, ss->y_max);
				// use full redraw
				reset_screen_dirty();

				topics ^= TOPIC_CHANGE_CONTENT_PART;
				topics |= TOPIC_CHANGE_CONTENT_ALL;
			} else {
				// is OK
				ss->i_max = ss->y_max * W + ss->x_max;
				ss->index = W*ss->y_min + ss->x_min;
				seri_dbg("Partial! X %d..%d, Y %d..%d, i_max %d", ss->x_min, ss->x_max, ss->y_min, ss->y_max, ss->i_max);
			}
		}

		if (topics & TOPIC_CHANGE_CONTENT_ALL) {
			// this is a no-clean request, do not purge
			// it's also always a full-screen repaint
			ss->partial = false;
			ss->index = 0;
			ss->i_max = W*H-1;
			ss->x_min = 0;
			ss->x_max = W-1;
			ss->y_min = 0;
			ss->y_max = H-1;
			seri_dbg("Full redraw!");
		}

		ss->i_start = ss->index;

		if ((topics & (TOPIC_CHANGE_CONTENT_ALL | TOPIC_CHANGE_CONTENT_PART)) && !(topics & TOPIC_FLAG_NOCLEAN)) {
			reset_screen_dirty();
		}

		ss->topics = topics;
		ss->last_topic = 0; // to be filled
		ss->current_topic = 0; // to be filled
		strncpy(ss->lastChar, " ", 4);

		bufput_c('U'); // - stands for "update"

		bufput_utf8(topics);

		if (ss->partial) {
			// advance to the first char we want to send
		}
	}

	int begun_topic = 0;
	int prev_topic = 0;

#define BEGIN_TOPIC(topic, size) \
	if (ss->last_topic == prev_topic) { \
		begun_topic = (topic); \
		if (ss->topics & (topic)) { \
            if (remain < (size)) return HTTPD_CGI_MORE;

#define END_TOPIC \
        } \
		ss->last_topic = begun_topic; \
		ss->current_topic = 0; \
	} \
	prev_topic = begun_topic;

	if (ss->current_topic == 0) {
		BEGIN_TOPIC(TOPIC_CHANGE_SCREEN_OPTS, 32+1)
			bufput_c(TOPICMARK_SCREEN_OPTS);

			bufput_utf8(H);
			bufput_utf8(W);
			bufput_utf8(termconf_live.theme);

			bufput_utf8(termconf_live.default_fg & 0xFFFF);
			bufput_utf8((termconf_live.default_fg >> 16) & 0xFFFF);

			bufput_utf8(termconf_live.default_bg & 0xFFFF);
			bufput_utf8((termconf_live.default_bg >> 16) & 0xFFFF);

			bufput_utf8(
				(scr.cursor_visible << 0) |
				(termconf_live.debugbar << 1) | // debugbar - this was previously "hanging"
				(scr.cursors_alt_mode << 2) |
				(scr.numpad_alt_mode << 3) |
				(termconf_live.fn_alt_mode << 4) |
				((mouse_tracking.mode > MTM_NONE) << 5) | // disables context menu
				((mouse_tracking.mode >= MTM_NORMAL) << 6) | // disables selecting
				(termconf_live.show_buttons << 7) |
				(termconf_live.show_config_links << 8) |
				((termconf_live.cursor_shape & 0x07) << 9) | // 9,10,11 - cursor shape based on DECSCUSR
				(termconf_live.crlf_mode << 12) |
				(scr.bracketed_paste << 13) |
				(scr.reverse_video << 14)
			);
		END_TOPIC

		BEGIN_TOPIC(TOPIC_CHANGE_TITLE, TERM_TITLE_LEN+4+1)
			bufput_c(TOPICMARK_TITLE);

			int len = (int) strlen(termconf_live.title);
			if (len > 0) {
				memcpy(bb, termconf_live.title, len);
				bb += len;
				remain -= len;
			}
			bufput_c('\x01');
		END_TOPIC

		BEGIN_TOPIC(TOPIC_CHANGE_BUTTONS, (TERM_BTN_LEN+4)*TERM_BTN_COUNT+1+4)
			bufput_c(TOPICMARK_BUTTONS);

			bufput_utf8(TERM_BTN_COUNT);

			for (int i = 0; i < TERM_BTN_COUNT; i++) {
				int len = (int) strlen(termconf_live.btn[i]);
				if (len > 0) {
					memcpy(bb, termconf_live.btn[i], len);
					bb += len;
					remain -= len;
				}
				bufput_c('\x01');
			}
		END_TOPIC

		BEGIN_TOPIC(TOPIC_CHANGE_BACKDROP, TERM_BACKDROP_LEN+1+1)
			bufput_c(TOPICMARK_BACKDROP);

			int len = (int) strlen(termconf_live.backdrop);
			if (len > 0) {
				memcpy(bb, termconf_live.backdrop, len);
				bb += len;
				remain -= len;
			}
			bufput_c('\x01');
		END_TOPIC

		BEGIN_TOPIC(TOPIC_INTERNAL, 45)
			bufput_c(TOPICMARK_DEBUG);
			// General flags
			bufput_utf8(
				(scr.insert_mode << 0) |
				(cursor.conceal << 1) |
				(cursor.auto_wrap << 2) |
				(cursor.reverse_wrap << 3) |
				(cursor.origin_mode << 4) |
				(cursor_saved << 5) |
				(state_backup.alternate_active << 6)
			);
			bufput_utf8(cursor.attrs);
			bufput_utf8(scr.vm0);
			bufput_utf8(scr.vm1);
			bufput_utf8(cursor.charsetN);
			bufput_c(cursor.charset0);
			bufput_c(cursor.charset1);
			bufput_utf8(system_get_free_heap_size());
			bufput_utf8(term_active_clients);
		END_TOPIC

		BEGIN_TOPIC(TOPIC_BELL, 1)
			bufput_c(TOPICMARK_BELL);
		END_TOPIC

		BEGIN_TOPIC(TOPIC_CHANGE_CURSOR, 13)
			bufput_c(TOPICMARK_CURSOR);
			bufput_utf8(cursor.y);
			bufput_utf8(cursor.x);
			bufput_utf8(
				(cursor.hanging << 0)
			);
		END_TOPIC

		if (ss->last_topic == TOPIC_CHANGE_CURSOR) {
			// now we can begin any of the two screen sequences

			if (ss->topics & TOPIC_CHANGE_CONTENT_ALL) {
				ss->current_topic = TOPIC_CHANGE_CONTENT_ALL;
			}
			if (ss->topics & TOPIC_CHANGE_CONTENT_PART) {
				ss->current_topic = TOPIC_CHANGE_CONTENT_PART;
			}

			if (ss->current_topic == 0) {
				// no screen mode - wrap it up
				goto ser_done;
			}

			// start the screen section
		}
	}

#define INC_I() do { \
		i++; \
		if (ss->partial) {\
			if (i%W == 0) i += (ss->x_min);\
			else if (i%W > ss->x_max) i += (W - ss->x_max + ss->x_min - 1);\
        } \
	} while (0)

	// screen contents
	int i = ss->index;
	if (i == ss->i_start) {
		bufput_c(TOPICMARK_SCREEN); // desired update mode is in `ss->current_topic`
		bufput_utf8(ss->y_min); // Y0
		bufput_utf8(ss->x_min); // X0
		bufput_utf8(ss->y_max - ss->y_min + 1); // height
		bufput_utf8(ss->x_max - ss->x_min + 1); // width
		ss->index = 0;
		ss->lastBg = 0;
		ss->lastFg = 0;
		ss->lastLiveBg = 0;
		ss->lastLiveFg = 0;
		ss->lastAttrs = 0;
		ss->lastCharLen = 0;
		ss->lastSymbol = 0;
		ss->first = 1;
	}
	while(i <= ss->i_max && remain > 12) {
		cell = cell0 = &screen[i];

		int repCnt = 0;

		if (!ss->first) {
			// Count how many times same as previous
			while (i <= ss->i_max
				   && cell->fg == ss->lastFg
				   && cell->bg == ss->lastBg
				   && cell->attrs == ss->lastAttrs
				   && cell->symbol == ss->lastSymbol) {
				// Repeat
				repCnt++;
				INC_I();
				cell = &screen[i]; // it can go outside the allocated memory here if we went over the top
			}
		}

		if (repCnt == 0) {
			// No repeat - first occurrence
			bool changeAttrs = ss->first || (cell0->attrs != ss->lastAttrs);
			bool changeFg = (cell0->fg != ss->lastLiveFg) && (cell0->attrs & ATTR_FG);
			bool changeBg = (cell0->bg != ss->lastLiveBg) && (cell0->attrs & ATTR_BG);
			bool changeColors = ss->first || (changeFg && changeBg);
			Color fg, bg;
			ss->first = false;

			// Reverse fg and bg if we're in global reverse mode
			fg = cell0->fg;
			bg = cell0->bg;

			if (changeColors) {
				bufput_t_utf8(SEQ_TAG_COLORS, bg<<8 | fg);
			}
			else if (changeFg) {
				bufput_t_utf8(SEQ_TAG_FG, fg);
			}
			else if (changeBg) {
				bufput_t_utf8(SEQ_TAG_BG, bg);
			}

			if (changeAttrs) {
				if (cell0->attrs) {
					bufput_t_utf8(SEQ_TAG_ATTRS, cell0->attrs);
				} else {
					bufput_c(SEQ_TAG_ATTRS_0);
				}
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
			if (cell0->attrs & ATTR_FG) ss->lastLiveFg = cell0->fg;
			if (cell0->attrs & ATTR_BG) ss->lastLiveBg = cell0->bg;
			ss->lastAttrs = cell0->attrs;
			ss->lastSymbol = cell0->symbol;

			INC_I();
		} else {
			// last character was repeated repCnt times
			int savings = ss->lastCharLen*repCnt;
			if (savings > 2) {
				// Repeat count
				bufput_t_utf8(SEQ_TAG_REPEAT, repCnt);
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
	if (i >= ss->i_max) goto ser_done;

	// MORE TO WRITE...
	bufput_c('\0'); // terminate the string
	return HTTPD_CGI_MORE;

ser_done:
	bufput_c('\0'); // terminate the string
	return HTTPD_CGI_DONE;
}
//endregion

#if 0
printf("MSG: ");
	for (int j=0;j<bb-buffer;j++) {
		printf("%02X ", buffer[j]);
	}
	printf("\n");
#endif
