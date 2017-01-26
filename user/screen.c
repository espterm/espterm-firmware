#include <esp8266.h>
#include <httpd.h>
#include "screen.h"

//region Data structures

/**
 * Highest permissible value of the color attribute
 */
#define COLOR_MAX 15

/**
 * Screen cell data type
 */
typedef struct {
	char c;
	Color fg;
	Color bg;
} Cell;

/**
 * The screen data array
 */
static Cell screen[MAX_SCREEN_SIZE];

/**
 * Cursor position and attributes
 */
static struct {
	Coordinate x;    //!< X coordinate
	Coordinate y;    //!< Y coordinate
	bool visible;    //!< Visible
	bool inverse;    //!< Inverse colors
	Color fg;        //!< Foreground color for writing
	Color bg;        //!< Background color for writing
} cursor;

/**
 * Saved cursor position, used with the SCP RCP commands
 */
static struct {
	Coordinate x;
	Coordinate y;
} cursor_sav;

/**
 * Active screen width
 */
static Coordinate W = SCREEN_DEF_W;

/**
 * Active screen height
 */
static Coordinate H = SCREEN_DEF_H;

// XXX volatile is probably not needed
static volatile int notifyLock = 0;

//endregion

//region Helpers

#define NOTIFY_LOCK()   do { \
							notifyLock++; \
						} while(0)

#define NOTIFY_DONE()   do { \
							if (notifyLock > 0) notifyLock--; \
							if (notifyLock == 0) screen_notifyChange(); \
						} while(0)

/**
 * Reset a cell
 */
static inline void
cell_init(Cell *cell)
{
	cell->c = ' ';
	cell->fg = SCREEN_DEF_FG;
	cell->bg = SCREEN_DEF_BG;
}

/**
 * Clear range, inclusive
 */
static inline void
clear_range(unsigned int from, unsigned int to)
{
	for (unsigned int i = from; i <= to; i++) {
		cell_init(&screen[i]);
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
	cursor.fg = SCREEN_DEF_FG;
	cursor.bg = SCREEN_DEF_BG;
	cursor.visible = 1;
	cursor.inverse = 0;
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
	for (unsigned int i = 0; i < MAX_SCREEN_SIZE; i++) {
		cell_init(&screen[i]);
	}

	cursor_reset();
	NOTIFY_DONE();
}

/**
 * Reset the screen (only the visible area)
 */
void ICACHE_FLASH_ATTR
screen_reset(void)
{
	NOTIFY_LOCK();
	screen_clear(CLEAR_ALL);
	cursor_reset();
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

//endregion

//region Screen manipulation

/**
 * Change the screen size
 *
 * @param w - new width
 * @param h - new height
 */
void ICACHE_FLASH_ATTR
screen_resize(Coordinate w, Coordinate h)
{
	NOTIFY_LOCK();
	// sanitize
	if (w < 1) w = 1;
	if (h < 1) h = 1;

	W = w;
	H = h;
	screen_reset();
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
screen_cursor_set(Coordinate x, Coordinate y)
{
	NOTIFY_LOCK();
	if (x >= W) x = W - 1;
	if (y >= H) y = H - 1;
	cursor.x = x;
	cursor.y = y;
	NOTIFY_DONE();
}

/**
 * Set cursor X position
 */
void ICACHE_FLASH_ATTR
screen_cursor_set_x(Coordinate x)
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
screen_cursor_set_y(Coordinate y)
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
screen_cursor_move(int dx, int dy)
{
	NOTIFY_LOCK();
	int move;

	cursor.x += dx;
	cursor.y += dy;
	if (cursor.x >= W) cursor.x = W - 1;
	if (cursor.x < 0) cursor.x = 0;

	if (cursor.y < 0) {
		move = -cursor.y;
		cursor.y = 0;
		screen_scroll_down((unsigned int)move);
	}

	if (cursor.y >= H) {
		move = cursor.y - (H - 1);
		cursor.y = H - 1;
		screen_scroll_up((unsigned int)move);
	}

	NOTIFY_DONE();
}

/**
 * Save the cursor pos
 */
void ICACHE_FLASH_ATTR
screen_cursor_save(void)
{
	cursor_sav.x = cursor.x;
	cursor_sav.y = cursor.y;
}

/**
 * Restore the cursor pos
 */
void ICACHE_FLASH_ATTR
screen_cursor_restore(void)
{
	NOTIFY_LOCK();
	cursor.x = cursor_sav.x;
	cursor.y = cursor_sav.y;
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
 */
void ICACHE_FLASH_ATTR
screen_set_bright_fg(void)
{
	cursor.fg = (cursor.fg % 8) + 8;
}

//endregion

/**
 * Set a character in the cursor color, move to right with wrap.
 */
void ICACHE_FLASH_ATTR
screen_putchar(char ch)
{
	NOTIFY_LOCK();

	Cell *c = &screen[cursor.x + cursor.y * W];

	// Special treatment for CRLF
	switch (ch) {
		case '\r':
			screen_cursor_set_x(0);
			goto done;

		case '\n':
			screen_cursor_move(0, 1);
			goto done;

		case 8: // BS
			if (cursor.x > 0) cursor.x--;
			// erase target cell
			c = &screen[cursor.x + cursor.y * W];
			c->c = ' ';
			goto done;

		case 9: // TAB
			c->c = ' ';
			// nested recurs >:( but it's ok
			screen_putchar(' ');
			screen_putchar(' ');
			screen_putchar(' ');
			screen_putchar(' ');
			goto done;

		default:
			if (ch < ' ') {
				// Discard
				warn("Ignoring control char %d", (int)ch);
				goto done;
			}
	}

	c->c = ch;

	if (cursor.inverse) {
		c->fg = cursor.bg;
		c->bg = cursor.fg;
	} else {
		c->fg = cursor.fg;
		c->bg = cursor.bg;
	}

	cursor.x++;
	// X wrap
	if (cursor.x >= W) {
		cursor.x = 0;
		cursor.y++;
		// Y wrap
		if (cursor.y > H-1) {
			// Scroll up, so we have space for writing
			screen_scroll_up(1);
			cursor.y = H-1;
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
	char lastChar;
	int index;
};

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
		ss->lastChar = '\0';

		bufprint("{\"w\":%d,\"h\":%d,\"x\":%d,\"y\":%d,\"cv\":%d,\"screen\":\"", W, H, cursor.x, cursor.y, cursor.visible);
	}

	int i = ss->index;
	while(i < W*H && remain > 6) {
		cell = cell0 = &screen[i];

		// Count how many times same as previous
		int repCnt = 0;
		while (i < W*H
		       && cell->fg == ss->lastFg
		       && cell->bg == ss->lastBg
		       && cell->c == ss->lastChar) {
			// Repeat
			repCnt++;
			cell = &screen[++i];
		}

		if (repCnt == 0) {
			if (cell0->fg == ss->lastFg && cell0->bg == ss->lastBg) {
				// same colors as previous
				bufprint(",%c", cell0->c);
			} else {
				bufprint("%X%X%c", cell0->fg, cell0->bg, cell0->c);
			}

			ss->lastFg = cell0->fg;
			ss->lastBg = cell0->bg;
			ss->lastChar = cell0->c;

			i++;
		} else {
			char code;
			if(repCnt<10) {
				code = 'r';
			} else if(repCnt<100) {
				code = 's';
			} else if(repCnt<1000) {
				code = 't';
			} else {
				code = 'u';
			}

			bufprint("%c%d", code, repCnt);
		}
	}

	ss->index = i;

	if (i < W*H-1) {
		return HTTPD_CGI_MORE;
	}

	if (remain >= 3) {
		bufprint("\"}");
		return HTTPD_CGI_DONE;
	} else {
		return HTTPD_CGI_MORE;
	}
}

//endregion
