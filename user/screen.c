#include <esp8266.h>
#include <httpd.h>
#include "screen.h"
#include "persist.h"

//region Data structures

TerminalConfigBundle * const termconf = &persist.current.termconf;
TerminalConfigBundle termconf_scratch;

#define W termconf_scratch.width
#define H termconf_scratch.height

/**
 * Restore hard defaults
 */
void terminal_restore_defaults(void)
{
	termconf->default_bg = 0;
	termconf->default_fg = 7;
	termconf->width = 26;
	termconf->height = 10;
	termconf->parser_tout_ms = 10;
	sprintf(termconf->title, "ESPTerm");
	for(int i=1; i <= 5; i++) {
		sprintf(termconf->btn[i-1], "%d", i);
	}
}

/**
 * Apply settings after eg. restore from defaults
 */
void terminal_apply_settings(void)
{
	terminal_apply_settings_noclear();
	screen_init();
}

void terminal_apply_settings_noclear(void)
{
	memcpy(&termconf_scratch, termconf, sizeof(TerminalConfigBundle));
	if (W*H >= MAX_SCREEN_SIZE) {
		error("BAD SCREEN SIZE: %d rows x %d cols", H, W);
		error("reverting terminal settings to default");
		terminal_restore_defaults();
		persist_store();
		memcpy(&termconf_scratch, termconf, sizeof(TerminalConfigBundle));
		screen_init();
	}
}

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
	bool bold : 1;
} Cell;

/**
 * The screen data array
 */
static Cell screen[MAX_SCREEN_SIZE];

/**
 * Cursor position and attributes
 */
static struct {
	int x;    //!< X coordinate
	int y;    //!< Y coordinate
	bool visible;    //!< Visible
	bool inverse;    //!< Inverse colors
	bool autowrap;   //!< Wrapping when EOL
	bool bold;       //!< Bold style
	Color fg;        //!< Foreground color for writing
	Color bg;        //!< Background color for writing
} cursor;

/**
 * Saved cursor position, used with the SCP RCP commands
 */
static struct {
	int x;
	int y;

	// optionally saved attrs
	bool withAttrs;
	bool inverse;
	Color fg;
	Color bg;
} cursor_sav;

// XXX volatile is probably not needed
static volatile int notifyLock = 0;

//endregion

//region Helpers

#define NOTIFY_LOCK()   do { \
							notifyLock++; \
						} while(0)

#define NOTIFY_DONE()   do { \
							if (notifyLock > 0) notifyLock--; \
							if (notifyLock == 0) screen_notifyChange(CHANGE_CONTENT); \
						} while(0)

/**
 * Clear range, inclusive
 */
static inline void
clear_range(unsigned int from, unsigned int to)
{
	if (to >= W*H) to = W*H-1;
	Color fg = cursor.inverse ? cursor.bg : cursor.fg;
	Color bg = cursor.inverse ? cursor.fg : cursor.bg;

	Cell sample;
	sample.c[0] = ' ';
	sample.c[1] = 0;
	sample.c[2] = 0;
	sample.c[3] = 0;
	sample.fg = fg;
	sample.bg = bg;
	sample.bold = false;

	for (unsigned int i = from; i <= to; i++) {
		memcpy(&screen[i], &sample, sizeof(Cell));
	}
}

/**
 * Reset the cursor position & colors
 */
static void ICACHE_FLASH_ATTR
cursor_reset(void)
{
	cursor.x = 0;
	cursor.y = 0;
	cursor.fg = termconf_scratch.default_fg;
	cursor.bg = termconf_scratch.default_bg;
	cursor.visible = 1;
	cursor.inverse = 0;
	cursor.autowrap = 1;
	cursor.bold = 0;
}

//endregion

//region Screen clearing

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
 * Reset the screen (only the visible area)
 */
void ICACHE_FLASH_ATTR
screen_reset(void)
{
	NOTIFY_LOCK();
	cursor_reset();
	screen_clear(CLEAR_ALL);
	NOTIFY_DONE();
}

/**
 * Reset the cursor
 */
void ICACHE_FLASH_ATTR
screen_reset_cursor(void)
{
	NOTIFY_LOCK();
	cursor.fg = termconf_scratch.default_fg;
	cursor.bg = termconf_scratch.default_bg;
	cursor.inverse = 0;
	cursor.bold = 0;
	NOTIFY_DONE();
}

/**
 * Clear screen area
 */
void ICACHE_FLASH_ATTR
screen_clear(ClearMode mode)
{
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
screen_fill_with_E(void)
{
	NOTIFY_LOCK();
	Cell sample;
	sample.c[0] = 'E';
	sample.c[1] = 0;
	sample.c[2] = 0;
	sample.c[3] = 0;
	sample.fg = termconf_scratch.default_fg;
	sample.bg = termconf_scratch.default_bg;
	sample.bold = false;

	for (unsigned int i = 0; i <= W*H-1; i++) {
		memcpy(&screen[i], &sample, sizeof(Cell));
	}
	NOTIFY_DONE();
}

//endregion

//region Screen manipulation

void screen_insert_lines(unsigned int lines)
{
	NOTIFY_LOCK();
	// TODO
	NOTIFY_DONE();
}

void screen_delete_lines(unsigned int lines)
{
	NOTIFY_LOCK();
	// TODO
	NOTIFY_DONE();
}

void screen_insert_characters(unsigned int count)
{
	NOTIFY_LOCK();
	// TODO
	NOTIFY_DONE();
}

void screen_delete_characters(unsigned int count)
{
	NOTIFY_LOCK();
	// TODO
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

/**
 * Shift screen upwards
 */
void ICACHE_FLASH_ATTR
screen_scroll_up(unsigned int lines)
{
	NOTIFY_LOCK();
	if (lines >= H - 1) {
		screen_clear(CLEAR_ALL);
		goto done;
	}

	// bad cmd
	if (lines == 0) {
		goto done;
	}

	int y;
	for (y = 0; y < H - lines; y++) {
		memcpy(screen + y * W, screen + (y + lines) * W, W * sizeof(Cell));
	}

	clear_range(y * W, W * H - 1);

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
	if (lines >= H - 1) {
		screen_clear(CLEAR_ALL);
		goto done;
	}

	// bad cmd
	if (lines == 0) {
		goto done;
	}

	int y;
	for (y = H-1; y >= lines; y--) {
		memcpy(screen + y * W, screen + (y - lines) * W, W * sizeof(Cell));
	}

	clear_range(0, lines * W-1);
done:
	NOTIFY_DONE();
}

//endregion

//region Cursor manipulation

/**
 * Set cursor position
 */
void ICACHE_FLASH_ATTR
screen_cursor_set(int y, int x)
{
	NOTIFY_LOCK();
	if (x >= W) x = W - 1;
	if (y >= H) y = H - 1;
	cursor.x = x;
	cursor.y = y;
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
	cursor.x = x;
	NOTIFY_DONE();
}

/**
 * Set cursor Y position
 */
void ICACHE_FLASH_ATTR
screen_cursor_set_y(int y)
{
	NOTIFY_LOCK();
	if (y >= H) y = H - 1;
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

	cursor.x += dx;
	cursor.y += dy;
	if (cursor.x >= (int)W) cursor.x = W - 1;
	if (cursor.x < (int)0) cursor.x = 0;

	if (cursor.y < 0) {
		move = -cursor.y;
		cursor.y = 0;
		if (scroll) screen_scroll_down((unsigned int)move);
	}

	if (cursor.y >= H) {
		move = cursor.y - (H - 1);
		cursor.y = H - 1;
		if (scroll) screen_scroll_up((unsigned int)move);
	}

	NOTIFY_DONE();
}

/**
 * Save the cursor pos
 */
void ICACHE_FLASH_ATTR
screen_cursor_save(bool withAttrs)
{
	cursor_sav.x = cursor.x;
	cursor_sav.y = cursor.y;

	cursor_sav.withAttrs = withAttrs;

	if (withAttrs) {
		cursor_sav.fg = cursor.fg;
		cursor_sav.bg = cursor.bg;
		cursor_sav.inverse = cursor.inverse;
	} else {
		cursor_sav.fg = termconf_scratch.default_fg;
		cursor_sav.bg = termconf_scratch.default_bg;
		cursor_sav.inverse = 0;
	}
}

/**
 * Restore the cursor pos
 */
void ICACHE_FLASH_ATTR
screen_cursor_restore(bool withAttrs)
{
	NOTIFY_LOCK();
	cursor.x = cursor_sav.x;
	cursor.y = cursor_sav.y;

	if (withAttrs) {
		cursor.fg = cursor_sav.fg;
		cursor.bg = cursor_sav.bg;
		cursor.inverse = cursor_sav.inverse;
	}

	NOTIFY_DONE();
}

/**
 * Enable cursor display
 */
void ICACHE_FLASH_ATTR
screen_cursor_enable(bool enable)
{
	NOTIFY_LOCK();
	cursor.visible = enable;
	NOTIFY_DONE();
}

/**
 * Enable autowrap
 */
void ICACHE_FLASH_ATTR
screen_wrap_enable(bool enable)
{
	NOTIFY_LOCK();
	cursor.autowrap = enable;
	NOTIFY_DONE();
}

//endregion

//region Colors

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
 * Set cursor foreground and background color
 */
void ICACHE_FLASH_ATTR
screen_set_colors(Color fg, Color bg)
{
	screen_set_fg(fg);
	screen_set_bg(bg);
}

/**
 * Invert colors
 */
void ICACHE_FLASH_ATTR
screen_inverse(bool inverse)
{
	cursor.inverse = inverse;
}

/**
 * Make foreground bright.
 *
 * This relates to the '1' SGR command which originally means
 * "bold font". We interpret that as "Bright", similar to other
 * terminal emulators.
 *
 * Note that the bright colors can be used without bold using the 90+ codes
 */
void ICACHE_FLASH_ATTR
screen_set_bold(bool bold)
{
	if (!bold) {
		cursor.fg = (Color) (cursor.fg % 8);
	} else {
		cursor.fg = (Color) ((cursor.fg % 8) + 8); // change anything to the bright colors
	}
	cursor.bold = bold;
}

//endregion

/**
 * Check if coords are in range
 *
 * @param y
 * @param x
 * @return OK
 */
bool ICACHE_FLASH_ATTR screen_isCoordValid(int y, int x)
{
	return x >= 0 && y >= 0 && x < W && y < H;
}

/**
 * Set a character in the cursor color, move to right with wrap.
 */
void ICACHE_FLASH_ATTR
screen_putchar(const char *ch)
{
	NOTIFY_LOCK();

	Cell *c = &screen[cursor.x + cursor.y * W];

	// Special treatment for CRLF
	switch (ch[0]) {
		case '\r':
			screen_cursor_set_x(0);
			goto done;

		case '\n':
			screen_cursor_move(1, 0, true); // can scroll
			goto done;

		case 8: // BS
			if (cursor.x > 0) {
				cursor.x--;
			} else {
				// wrap around start of line
				if (cursor.autowrap && cursor.y>0) {
					cursor.x=W-1;
					cursor.y--;
				}
			}
			// erase target cell
			c = &screen[cursor.x + cursor.y * W];
			c->c[0] = ' ';
			c->c[1] = 0;
			c->c[2] = 0;
			c->c[3] = 0;
			goto done;

		case 9: // TAB
			if (cursor.x<((W-1)-(W-1)%4)) {
				c->c[0] = ' ';
				c->c[1] = 0;
				c->c[2] = 0;
				c->c[3] = 0;
				do {
					screen_putchar(" ");
				} while(cursor.x%4!=0);
			}
			goto done;

		default:
			if (ch[0] < ' ') {
				// Discard
				warn("Ignoring control char %d", (int)ch);
				goto done;
			}
	}

	// copy unicode char
	strncpy(c->c, ch, 4);

	if (cursor.inverse) {
		c->fg = cursor.bg;
		c->bg = cursor.fg;
	} else {
		c->fg = cursor.fg;
		c->bg = cursor.bg;
	}
	c->bold = cursor.bold;

	cursor.x++;
	// X wrap
	if (cursor.x >= W) {
		if (cursor.autowrap) {
			cursor.x = 0;
			cursor.y++;
			// Y wrap
			if (cursor.y > H - 1) {
				// Scroll up, so we have space for writing
				screen_scroll_up(1);
				cursor.y = H - 1;
			}
		} else {
			cursor.x = W - 1;
		}
	}

done:
	NOTIFY_DONE();
}


//region Serialization

#if 0
/**
 * Debug dump
 */
void screen_dd(void)
{
	for (int y = 0; y < H; y++) {
		for (int x = 0; x < W; x++) {
			Cell *cell = &screen[y * W + x];

			// FG
			printf("\033[");
			if (cell->fg > 7) {
				printf("%d", 90 + cell->fg - 8);
			} else {
				printf("%d", 30 + cell->fg);
			}
			printf("m");

			// BG
			printf("\033[");
			if (cell->bg > 7) {
				printf("%d", 100 + cell->bg - 8);
			} else {
				printf("%d", 40 + cell->bg);
			}
			printf("m");

			printf("%c", cell->c);
		}
		printf("\033[0m\n");
	}
}
#endif

struct ScreenSerializeState {
	Color lastFg;
	Color lastBg;
	bool lastBold;
	char lastChar[4];
	int index;
};

void ICACHE_FLASH_ATTR
encode2B(u16 number, WordB2 *stru)
{
	stru->lsb = (u8) (number % 127);
	stru->msb = (u8) ((number - stru->lsb) / 127 + 1);
	stru->lsb += 1;
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
		ss->lastBold = false;
		memset(ss->lastChar, 0, 4); // this ensures the first char is never "repeat"

		encode2B((u16) H, &w1);
		encode2B((u16) W, &w2);
		encode2B((u16) cursor.y, &w3);
		encode2B((u16) cursor.x, &w4);
		encode2B((u16) (
			cursor.fg |
			(cursor.bg<<4) |
			(cursor.bold?0x100:0) |
			(cursor.visible?0x200:0))
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
		       && cell->bold == ss->lastBold
		       && strneq(cell->c, ss->lastChar, 4)) {
			// Repeat
			repCnt++;
			cell = &screen[++i];
		}

		if (repCnt == 0) {
			// No repeat
			if (cell0->bold != ss->lastBold || cell0->fg != ss->lastFg || cell0->bg != ss->lastBg) {
				encode2B((u16) (
							 cell0->fg |
							 (cell0->bg<<4) |
							 (cell0->bold?0x100:0))
					, &w1);
				bufprint("\x01%c%c", w1.lsb, w1.msb);
			}

			// copy the symbol, until first 0 or reached 4 bytes
			char c;
			int j = 0;
			while ((c = cell->c[j++]) != 0 && j < 4) {
				bufprint("%c", c);
			}

			ss->lastFg = cell0->fg;
			ss->lastBg = cell0->bg;
			ss->lastBold = cell0->bold;
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
