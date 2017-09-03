#include <esp8266.h>
#include <httpd.h>
#include "screen.h"
#include "persist.h"
#include "sgr.h"
#include "ascii.h"
#include "apars_logging.h"

TerminalConfigBundle * const termconf = &persist.current.termconf;
TerminalConfigBundle termconf_scratch;

// forward declare
static void utf8_remap(char* out, char g, char charset);

#define W termconf_scratch.width
#define H termconf_scratch.height

/**
 * Highest permissible value of the color attribute
 */
#define COLOR_MAX 15

/**
 * Screen cell data type (16 bits)
 */
typedef struct __attribute__((packed)){
	char c[4]; // space for a full unicode character
	Color fg : 4;
	Color bg : 4;
	u8 attrs;
} Cell;

/**
 * The screen data array
 */
static Cell screen[MAX_SCREEN_SIZE];


#define TABSTOP_WORDS 5
/**
 * Tab stops bitmap
 */
static u32 tab_stops[TABSTOP_WORDS];

/**
 * Screen state structure
 */
static struct {
	bool numpad_alt_mode;   //!< Application Mode - affects how user input of control keys is sent
	bool cursors_alt_mode; //!< Application mode for cursor keys

	bool newline_mode; // LNM - CR automatically sends LF
	bool reverse;

	// Vertical margin bounds (inclusive start/end of scrolling region)
	int vm0;
	int vm1;
} scr;

#define R0 scr.vm0
#define R1 scr.vm1
#define RH (scr.vm1 - scr.vm0 + 1)

typedef struct {
	int x;        //!< X coordinate
	int y;        //!< Y coordinate
	bool hanging; //!< xenl state - cursor half-wrapped

	// SGR
	bool inverse; //!< not in attrs bc it's applied server-side (not sent to browser)
	u8 attrs;
	Color fg;     //!< Foreground color for writing
	Color bg;     //!< Background color for writing

	// Other attribs
	int charsetN;
	char charset0;
	char charset1;
	bool wraparound;   //!< Wrapping when EOL
	bool origin_mode; // DECOM - absolute positioning is relative to vertical margins
	bool selective_erase; // TODO implement

	// Not saved/restored FIXME those should not be saved, but are (a bug?)
	bool insert_mode;    //!< Insert mode (move rest of the line to the right)
	bool visible; //!< Visible (not attribute, DEC special)
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
	termconf->default_bg = 0;
	termconf->default_fg = 7;
	termconf->width = SCR_DEF_WIDTH;
	termconf->height = SCR_DEF_HEIGHT;
	termconf->parser_tout_ms = SCR_DEF_PARSER_TOUT_MS;
	termconf->display_tout_ms = SCR_DEF_DISPLAY_TOUT_MS;
	termconf->fn_alt_mode = SCR_DEF_FN_ALT_MODE;
	sprintf(termconf->title, SCR_DEF_TITLE);
	for(int i=1; i <= 5; i++) {
		sprintf(termconf->btn[i-1], "%d", i);
	}
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
		dbg("termconf: Updating to version 1");
		termconf->display_cooldown_ms = SCR_DEF_DISPLAY_COOLDOWN_MS;
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

	memcpy(&termconf_scratch, termconf, sizeof(TerminalConfigBundle));
	if (W*H > MAX_SCREEN_SIZE) {
		error("BAD SCREEN SIZE: %d rows x %d cols", H, W);
		error("reverting terminal settings to default");
		terminal_restore_defaults();
		changed = true;
		memcpy(&termconf_scratch, termconf, sizeof(TerminalConfigBundle));
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
	NOTIFY_LOCK();
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
	cursor.visible = true;
	cursor.insert_mode = false;
	cursor.selective_erase = false;
	cursor.origin_mode = false;

	cursor.charsetN = 0;
	cursor.charset0 = CS_USASCII;
	cursor.charset1 = CS_DEC_SUPPLEMENTAL;
	cursor.wraparound = true;

	screen_reset_sgr();
}

/**
 * Reset the cursor (\e[0m)
 */
void ICACHE_FLASH_ATTR
screen_reset_sgr(void)
{
	cursor.fg = termconf_scratch.default_fg;
	cursor.bg = termconf_scratch.default_bg;
	cursor.attrs = 0;
	cursor.inverse = false;
}

/**
 * Reset the screen - called by ESC c
 */
void ICACHE_FLASH_ATTR
screen_reset(void)
{
	NOTIFY_LOCK();

	cursor_reset();

	scr.numpad_alt_mode = false;
	scr.cursors_alt_mode = false;
	scr.newline_mode = false;
	scr.reverse = false;

	scr.vm0 = 0;
	scr.vm1 = H-1;

	// size is left unchanged
	screen_clear(CLEAR_ALL);

	screen_clear_all_tabs();

	// Set initial tabstops
	for (int i = 0; i < TABSTOP_WORDS; i++) {
		tab_stops[i] = 0x80808080;
	}

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
	// TODO impl - backup/restore title, size, global attributes (not SGR)
}

//endregion

//region --- Tab stops ---

void ICACHE_FLASH_ATTR
screen_clear_all_tabs(void)
{
	memset(tab_stops, 0, sizeof(tab_stops));
}

void ICACHE_FLASH_ATTR
screen_set_tab(void)
{
	tab_stops[cursor.x/32] |= (1<<(cursor.x%32));
}

void ICACHE_FLASH_ATTR
screen_clear_tab(void)
{
	tab_stops[cursor.x/32] &= ~(1<<(cursor.x%32));
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
		u32 w = tab_stops[idx];
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
		u32 w = tab_stops[idx];
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
	sample.c[0] = ' ';
	sample.c[1] = 0;
	sample.c[2] = 0;
	sample.c[3] = 0;
	sample.fg = fg;
	sample.bg = bg;
	sample.attrs = 0;

	for (unsigned int i = from; i <= to; i++) {
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

void screen_insert_lines(unsigned int lines)
{
	if (!cursor_inside_region()) return; // can't insert if not inside region
	NOTIFY_LOCK();
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

void screen_delete_lines(unsigned int lines)
{
	if (!cursor_inside_region()) return; // can't delete if not inside region
	NOTIFY_LOCK();

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

void screen_insert_characters(unsigned int count)
{
	NOTIFY_LOCK();
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

void screen_delete_characters(unsigned int count)
{
	NOTIFY_LOCK();
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
	sample.c[0] = 'E';
	sample.c[1] = 0;
	sample.c[2] = 0;
	sample.c[3] = 0;
	sample.fg = termconf_scratch.default_fg;
	sample.bg = termconf_scratch.default_bg;
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
	NOTIFY_LOCK();
	// sanitize
	if (cols < 1 || rows < 1) {
		error("Screen size must be positive");
		goto done;
	}

	if (cols * rows > MAX_SCREEN_SIZE) {
		error("Max screen size exceeded");
		goto done;
	}

	W = cols;
	H = rows;
	screen_reset();
	done:
	NOTIFY_DONE();
}

void ICACHE_FLASH_ATTR
screen_set_title(const char *title)
{
	strncpy(termconf_scratch.title, title, TERM_TITLE_LEN);
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
	if (cursor.x < (int)0) cursor.x = 0;

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
//endregion

//region --- Attribute setting ---

/**
 * Enable cursor display
 */
void ICACHE_FLASH_ATTR
screen_set_cursor_visible(bool visible)
{
	NOTIFY_LOCK();
	cursor.visible = visible;
	NOTIFY_DONE();
}

/**
 * Enable autowrap
 */
void ICACHE_FLASH_ATTR
screen_wrap_enable(bool enable)
{
	cursor.wraparound = enable;
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

/**
 * Check if coords are in range - used for verifying mouse clicks
 *
 * @param y
 * @param x
 * @return OK
 */
bool ICACHE_FLASH_ATTR
screen_isCoordValid(int y, int x)
{
	return x >= 0 && y >= 0 && x < W && y < H;
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
	cursor.insert_mode = insert;
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
	scr.reverse = reverse;
	NOTIFY_DONE();
}

void ICACHE_FLASH_ATTR
screen_set_newline_mode(bool nlm)
{
	scr.newline_mode = nlm;
}

void ICACHE_FLASH_ATTR
screen_set_origin_mode(bool region_origin)
{
	cursor.origin_mode = region_origin;
	screen_cursor_set(0, 0);
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
}

//endregion

//region --- Printing ---

/**
 * Set a character in the cursor color, move to right with wrap.
 */
void ICACHE_FLASH_ATTR
screen_putchar(const char *ch)
{
	NOTIFY_LOCK();

	// clear "hanging" flag if not possible
	clear_invalid_hanging();

	// Special treatment for CRLF
	switch (ch[0]) {
		case CR:
			screen_cursor_set_x(0);
			goto done;

		case LF:
			screen_cursor_move(1, 0, true); // can scroll
			if (scr.newline_mode) {
				// like CR
				screen_cursor_set_x(0);
			}
			goto done;

		case BS:
			if (cursor.x > 0) {
				// backspace should go to col 79 if "hanging" after 80 (as if it never actually left the 80th col)
				cursor.hanging = false;
				cursor.x--;
			}
			// we should not wrap around, and backspace should not even clear the cell (verified in xterm)
			goto done;

		default:
			if (ch[0] < SP) {
				// Discard
				warn("Ignoring control char %d", (int)ch[0]);
				goto done;
			}
	}

	if (cursor.hanging) {
		// perform the scheduled wrap if hanging
		// if auto-wrap = off, it overwrites the last char
		if (cursor.wraparound) {
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
	if (cursor.x < W-1 && cursor.insert_mode) screen_insert_characters(1);

	if (ch[1] == 0 && ch[0] <= 0x7f) {
		// we have len=1 and ASCII
		utf8_remap(c->c, ch[0], (cursor.charsetN == 0) ? cursor.charset0 : cursor.charset1);
	}
	else {
		// copy unicode char
		strncpy(c->c, ch, 4);
	}

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

	done:
	NOTIFY_DONE();
}

/**
 * translates VT100 ACS escape codes to Unicode values.
 * Based on rxvt-unicode screen.C table.
 */
static const u16 codepage_0[] =
	{//	Unicode    ASCII   SYM
		0x2666, // 96	`	♦
		0x2592, // 97	a	▒
		0x2409, // 98	b	HT
		0x240c, // 99	c	FF
		0x240d, // 100	d	CR
		0x240a, // 101	e	LF
		0x00b0, // 102	f	°
		0x00b1, // 103	g	±
		0x2424, // 104	h	NL
		0x240b, // 105	i	VT
		0x2518, // 106	j	┘
		0x2510, // 107	k	┐
		0x250c, // 108	l	┌
		0x2514, // 109	m	└
		0x253c, // 110	n	┼
		0x23ba, // 111	o	⎺
		0x23bb, // 112	p	⎻
		0x2500, // 113	q	─
		0x23bc, // 114	r	⎼
		0x23bd, // 115	s	⎽
		0x251c, // 116	t	├
		0x2524, // 117	u	┤
		0x2534, // 118	v	┴
		0x252c, // 119	w	┬
		0x2502, // 120	x	│
		0x2264, // 121	y	≤
		0x2265, // 122	z	≥
		0x03c0, // 123	{	π
		0x2260, // 124	|	≠
		0x20a4, // 125	}	£
		0x00b7, // 126	~	·
	};

static const u16 codepage_1[] =
	{//	Unicode    ASCII   SYM  DOS
		0x263A,	// 33	!	☺	(1)  - low ASCII symbols from DOS, moved to +32
		0x263B,	// 34	"	☻	(2)
		0x2665,	// 35	#	♥	(3)
		0x2666,	// 36	$	♦	(4)
		0x2663,	// 37	%	♣	(5)
		0x2660,	// 38	&	♠	(6)
		0x2022,	// 39	'	•	(7)  - inverse dot and circle left out, can be done with SGR
	0x0,//0x231B,	// 40	(	⌛        - hourglass (timer icon)
		0x25CB,	// 41	)	○	(9)
		0x21AF,	// 42	*	↯        - electricity (lightning monitor...)
		0x266A,	// 43	+	♪	(13)
		0x266B,	// 44	,	♫	(14)
		0x263C,	// 45	-	☼	(15)
		0x2302,	// 46	.	⌂	(127)
	0x0,//0x2622,	// 47	/	☢         - radioactivity (geiger counter...)
		0x2591,	// 48	0	░	(176) - this block is kept aligned and ordered from DOS, moved -128
		0x2592,	// 49	1	▒	(177)
		0x2593,	// 50	2	▓	(178)
		0x2502,	// 51	3	│	(179)
		0x2524,	// 52	4	┤	(180)
		0x2561,	// 53	5	╡	(181)
		0x2562,	// 54	6	╢	(182)
		0x2556,	// 55	7	╖	(183)
		0x2555,	// 56	8	╕	(184)
		0x2563,	// 57	9	╣	(185)
		0x2551,	// 58	:	║	(186)
		0x2557,	// 59	;	╗	(187)
		0x255D,	// 60	<	╝	(188)
		0x255C,	// 61	=	╜	(189)
		0x255B,	// 62	>	╛	(190)
		0x2510,	// 63	?	┐	(191)
		0x2514,	// 64	@	└	(192)
		0x2534,	// 65	A	┴	(193)
		0x252C,	// 66	B	┬	(194)
		0x251C,	// 67	C	├	(195)
		0x2500,	// 68	D	─	(196)
		0x253C,	// 69	E	┼	(197)
		0x255E,	// 70	F	╞	(198)
		0x255F,	// 71	G	╟	(199)
		0x255A,	// 72	H	╚	(200)
		0x2554,	// 73	I	╔	(201)
		0x2569,	// 74	J	╩	(202)
		0x2566,	// 75	K	╦	(203)
		0x2560,	// 76	L	╠	(204)
		0x2550,	// 77	M	═	(205)
		0x256C,	// 78	N	╬	(206)
		0x2567,	// 79	O	╧	(207)
		0x2568,	// 80	P	╨	(208)
		0x2564,	// 81	Q	╤	(209)
		0x2565,	// 82	R	╥	(210)
		0x2559,	// 83	S	╙	(211)
		0x2558,	// 84	T	╘	(212)
		0x2552,	// 85	U	╒	(213)
		0x2553,	// 86	V	╓	(214)
		0x256B,	// 87	W	╫	(215)
		0x256A,	// 88	X	╪	(216)
		0x2518,	// 89	Y	┘	(217)
		0x250C,	// 90	Z	┌	(218)
		0x2588,	// 91	[	█	(219)
		0x2584,	// 92	\	▄	(220)
		0x258C,	// 93	]	▌	(221)
		0x2590,	// 94	^	▐	(222)
		0x2580,	// 95	_	▀	(223)
		0x2195,	// 96	`	↕	(18)  - moved from low DOS ASCII
		0x2191,	// 97	a	↑	(24)
		0x2193,	// 98	b	↓	(25)
		0x2192,	// 99	c	→	(26)
		0x2190,	// 100	d	←	(27)
		0x2194,	// 101	e	↔	(29)
		0x25B2,	// 102	f	▲	(30)
		0x25BC,	// 103	g	▼	(31)
		0x25BA,	// 104	h	►	(16)
		0x25C4,	// 105	i	◄	(17)
	0x0,//0x25E2,	// 106	j	◢         - added for slanted corners
	0x0,//0x25E3,	// 107	k	◣
	0x0,//0x25E4,	// 108	l	◤
	0x0,//0x25E5,	// 109	m	◥
		0x256D,	// 110	n	╭         - rounded corners
		0x256E,	// 111	o	╮
		0x256F,	// 112	p	╯
		0x2570,	// 113	q	╰
		0x0,	// 114	r             - free positions for future expansion
		0x0,	// 115	s
		0x0,	// 116	t
		0x0,	// 117	u
		0x0,	// 118	v
		0x0,	// 119	w
		0x0,	// 120	x
		0x0,	// 121	y
		0x0,	// 122	z
		0x0,	// 123	{
		0x0,	// 124	|
	0x0,//0x2714,	// 125	}	✔         - checkboxes or checklist items
	0x0,//0x2718,	// 126	~	✘
	};

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
		case CS_DEC_SUPPLEMENTAL: /* DEC Special Character & Line Drawing Set */
			if ((g >= 96) && (g < 0x7F)) {
				n = codepage_0[g - 96];
				if (n) utf = n;
			}
			break;

		case CS_DOS_437: /* ESPTerm Character Rom 1 */
			if ((g >= 33) && (g < 0x7F)) {
				n = codepage_1[g - 33];
				if (n) utf = n;
			}
			break;

		case CS_UKASCII: /* UK, replaces # with GBP */
			if (g == '#') utf = 0x20a4;
			break;

		default:
		case CS_USASCII:
			// No change
			break;
	}

	// Encode to UTF-8
	if (utf > 0x7F) {
		// formulas taken from: https://gist.github.com/yamamushi/5823402
		if ((utf >= 0x80) && (utf <= 0x07FF)) {
			// 2-byte unicode
			out[0] = (char) ((utf >> 0x06) ^ 0xC0);
			out[1] = (char) (((utf ^ 0xFFC0) | 0x80) & ~0x40);
			out[2]=0;
		}
		else {
			// 3-byte unicode
			out[0] = (char) (((utf ^ 0xFC0FFF) >> 0x0C) | 0xE0);
			out[1] = (char) ((((utf ^ 0xFFF03F) >> 0x06) | 0x80) & ~0x40);
			out[2] = (char) (((utf ^ 0xFFFC0) | 0x80) & ~0x40);
			out[3]=0;
		}
		// Missing 4-byte formulas :(
	} else {
		// low ASCII
		out[0] = (char) utf;
		out[1] = 0;
	}
}
//endregion

//region --- Serialization ---

struct ScreenSerializeState {
	Color lastFg;
	Color lastBg;
	bool lastAttrs;
	char lastChar[4];
	int index;
};

void ICACHE_FLASH_ATTR
encode2B(u16 number, WordB2 *stru)
{
	stru->lsb = (u8) (number % 127);
	number = (u16) ((number - stru->lsb) / 127);
	stru->lsb += 1;

	stru->msb = (u8) (number + 1);
}

void ICACHE_FLASH_ATTR
encode3B(u32 number, WordB3 *stru)
{
	stru->lsb = (u8) (number % 127);
	number = (number - stru->lsb) / 127;
	stru->lsb += 1;

	stru->msb = (u8) (number % 127);
	number = (number - stru->msb) / 127;
	stru->msb += 1;

	stru->xsb = (u8) (number + 1);
}

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
			termconf_scratch.title,
			termconf_scratch.btn[0],
			termconf_scratch.btn[1],
			termconf_scratch.btn[2],
			termconf_scratch.btn[3],
			termconf_scratch.btn[4]
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
	WordB2 w1, w2, w3, w4, w5;
	WordB3 lw1;

	size_t remain = buf_len; int used = 0;
	char *bb = buffer;

	// Ideally we'd use snprintf here!
#define bufprint(fmt, ...) do { \
			used = sprintf(bb, fmt, ##__VA_ARGS__); \
			if(used>0) { bb += used; remain -= used; } \
		} while(0)

	if (ss == NULL) {
		*data = ss = malloc(sizeof(struct ScreenSerializeState));
		ss->index = 0;
		ss->lastBg = 0;
		ss->lastFg = 0;
		ss->lastAttrs = 0;
		memset(ss->lastChar, 0, 4); // this ensures the first char is never "repeat"

		encode2B((u16) H, &w1);
		encode2B((u16) W, &w2);
		encode2B((u16) cursor.y, &w3);
		encode2B((u16) cursor.x, &w4);
		encode2B((u16) (
					 (cursor.visible ? 0x01 : 0) |
					 (cursor.hanging ? 0x02 : 0) |
					 (scr.cursors_alt_mode ? 0x04 : 0) |
					 (scr.numpad_alt_mode ? 0x08 : 0) |
					 (termconf->fn_alt_mode ? 0x10 : 0)
				 )
			, &w5);

		// H W X Y Attribs
		bufprint("S%c%c%c%c%c%c%c%c%c%c", w1.lsb, w1.msb, w2.lsb, w2.msb, w3.lsb, w3.msb, w4.lsb, w4.msb, w5.lsb, w5.msb);
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
			   && strneq(cell->c, ss->lastChar, 4)) {
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
			if (! scr.reverse) {
				fg = cell0->fg;
				bg = cell0->bg;
			}
			else {
				fg = cell0->bg;
				bg = cell0->fg;
			}

			if (!changeAttrs && changeColors) {
				encode2B(fg | (bg<<4), &w1);
				bufprint("\x03%c%c", w1.lsb, w1.msb);
			}
			else if (changeAttrs && !changeColors) {
				// attrs only
				encode2B(cell0->attrs, &w1);
				bufprint("\x04%c%c", w1.lsb, w1.msb);
			}
			else if (changeAttrs && changeColors) {
				// colors and attrs
				encode3B((u32) (fg | (bg<<4) | (cell0->attrs<<8)), &lw1);
				bufprint("\x01%c%c%c", lw1.lsb, lw1.msb, lw1.xsb);
			}

			// copy the symbol, until first 0 or reached 4 bytes
			char c;
			int j = 0;
			while ((c = cell->c[j++]) != 0 && j < 4) {
				bufprint("%c", c);
			}

			ss->lastFg = cell0->fg;
			ss->lastBg = cell0->bg;
			ss->lastAttrs = cell0->attrs;
			memcpy(ss->lastChar, cell0->c, 4);

			i++;
		} else {
			// Repeat count
			encode2B((u16) repCnt, &w1);
			bufprint("\x02%c%c", w1.lsb, w1.msb);
		}
	}

	ss->index = i;

	if (i < W*H-1) {
		return HTTPD_CGI_MORE;
	}

	return HTTPD_CGI_DONE;
}
//endregion
