#include <esp8266.h>
#include <httpd.h>
#include "screen.h"
#include "persist.h"

//region Data structures

TerminalConfigBundle * const termconf = &persist.current.termconf;
TerminalConfigBundle termconf_scratch;

// forward declare
static void utf8_remap(char* out, char g, char table);

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

/**
 * Cursor position and attributes
 */
static struct {
	int x;    //!< X coordinate
	int y;    //!< Y coordinate
	bool autowrap;   //!< Wrapping when EOL
	bool insert_mode; //!< Insert mode (move rest of the line to the right)

	// TODO use those for input processing
	bool kp_alternate;   //!< Application Mode - affects how user input of control keys is sent
	bool curs_alternate; //!< Application mode for cursor keys

	bool visible;    //!< Visible (not attribute, DEC special)
	bool inverse;
	u8 attrs;

	char charset0;
	char charset1;
	int charsetN;

	Color fg;        //!< Foreground color for writing
	Color bg;        //!< Background color for writing
} cursor;

/**
 * Saved cursor position, used with the SCP RCP commands
 */
static struct {
	int x;
	int y;
	// mark that attrs are saved
	bool withAttrs;
	u8 attrs;
	bool inverse; // attribute that's not in the bitfield
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

/** this is used when changing terminal settings that do not affect the screen size */
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
 * Clear range, inclusive
 */
static inline void
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
 * Reset the cursor position & colors
 */
static void ICACHE_FLASH_ATTR
cursor_reset(void)
{
	// TODO this should probably be public and invoked by "soft reset"

	cursor.x = 0;
	cursor.y = 0;
	cursor.fg = termconf_scratch.default_fg;
	cursor.bg = termconf_scratch.default_bg;
	cursor.visible = 1;
	cursor.autowrap = 1;
	cursor.attrs = 0;
	cursor.insert_mode = 0;

	cursor.kp_alternate = 0;
	cursor.curs_alternate = 0;

	cursor.charset0 = 'B';
	cursor.charset1 = '0';
	cursor.charsetN = 0;
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
	cursor.fg = termconf_scratch.default_fg;
	cursor.bg = termconf_scratch.default_bg;
	cursor.attrs = 0;
	cursor.inverse = false;
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
	sample.attrs = 0;

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
	// shift the following lines
	int targetStart = cursor.y + lines;
	if (targetStart >= H) {
		targetStart = H;
	} else {
		// do the moving
		for (int i = H-1; i >= targetStart; i--) {
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
	NOTIFY_LOCK();
	// shift lines up
	int targetEnd = H - 1 - lines;
	if (targetEnd <= cursor.y) {
		targetEnd = cursor.y;
	} else {
		// do the moving
		for (int i = cursor.y; i <= targetEnd; i++) {
			memcpy(screen+i*W, screen+(i+lines)*W, sizeof(Cell)*W);
		}
	}

	// clear the rest
	clear_range(targetEnd*W, W*H-1);
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
		cursor_sav.attrs = cursor.attrs;
	} else {
		cursor_sav.fg = termconf_scratch.default_fg;
		cursor_sav.bg = termconf_scratch.default_bg;
		cursor_sav.attrs = 0; // avoid leftovers if the wrong restore is used
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
		cursor.attrs = cursor_sav.attrs;
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

void screen_attr_enable(u8 attrs)
{
	cursor.attrs |= attrs;
}

void screen_attr_disable(u8 attrs)
{
	cursor.attrs &= ~attrs;
}

void screen_inverse_enable(bool ena)
{
	cursor.inverse = ena;
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


void ICACHE_FLASH_ATTR screen_set_charset_n(int Gx)
{
	if (Gx < 0 || Gx > 1) return; // bad n
	cursor.charsetN = Gx;
}

void ICACHE_FLASH_ATTR screen_set_charset(int Gx, char charset)
{
	if (Gx == 0) cursor.charset0 = charset;
	if (Gx == 1) cursor.charset1 = charset;
}

void ICACHE_FLASH_ATTR screen_set_insert_mode(bool insert)
{
	cursor.insert_mode = insert;
}

void ICACHE_FLASH_ATTR screen_set_keypad_application_mode(bool app_mode)
{
	cursor.kp_alternate = app_mode;
}

void ICACHE_FLASH_ATTR screen_set_cursor_application_mode(bool app_mode)
{
	cursor.curs_alternate = app_mode;
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
			}
			// we should not wrap around
			// and apparently backspace should not even clear the cell
			goto done;

		case 9: // TAB
			// TODO change to "go to next tab stop"
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
				warn("Ignoring control char %d", (int)ch[0]);
				goto done;
			}
	}

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

/**
 * translates VT100 ACS escape codes to Unicode values.
 * Based on rxvt-unicode screen.C table.
 */
static const u16 vt100_to_unicode[62] =
{
// ?       ?       ?       ?       ?       ?       ?
// A=UPARR B=DNARR C=RTARR D=LFARR E=FLBLK F=3/4BL G=SNOMN
	0x2191, 0x2193, 0x2192, 0x2190, 0x2588, 0x259a, 0x2603,
// H=      I=      J=      K=      L=      M=      N=
	0,      0,      0,      0,      0,      0,      0,
// O=      P=      Q=      R=      S=      T=      U=
	0,      0,      0,      0,      0,      0,      0,
// V=      W=      X=      Y=      Z=      [=      \=
	0,      0,      0,      0,      0,      0,      0,
// ?       ?       v->0    v->1    v->2    v->3    v->4
// ]=      ^=      _=SPC   `=DIAMN a=HSMED b=HT    c=FF
	0,      0,      0x0020, 0x25c6, 0x2592, 0x2409, 0x240c,
// v->5    v->6    v->7    v->8    v->9    v->a    v->b
// d=CR    e=LF    f=DEGRE g=PLSMN h=NL    i=VT    j=SL-BR
	0x240d, 0x240a, 0x00b0, 0x00b1, 0x2424, 0x240b, 0x2518,
// v->c    v->d    v->e    v->f    v->10   v->11   v->12
// k=SL-TR l=SL-TL m=SL-BL n=SL-+  o=SL-T1 p=SL-T2 q=SL-HZ
	0x2510, 0x250c, 0x2514, 0x253c, 0x23ba, 0x23bb, 0x2500,
// v->13   v->14   v->15   v->16   v->17   v->18   v->19
// r=SL-T4 s=SL-T5 t=SL-VR u=SL-VL v=SL-HU w=Sl-HD x=SL-VT
	0x23bc, 0x23bd, 0x251c, 0x2524, 0x2534, 0x252c, 0x2502,
// v->1a   v->1b   b->1c   v->1d   v->1e/a3 v->1f
// y=LT-EQ z=GT-EQ {=PI    |=NOTEQ }=POUND ~=DOT
	0x2264, 0x2265, 0x03c0, 0x2260, 0x20a4, 0x00b7
};

/**
 * UTF remap
 * @param out - output char[4]
 * @param g  - ASCII char
 * @param table - table name (0, A, B)
 */
static void ICACHE_FLASH_ATTR
utf8_remap(char *out, char g, char table)
{
	u16 utf = 0;

	switch (table)
	{
		case '0': /* DEC Special Character & Line Drawing Set */
			if ((g >= 0x41) && (g <= 0x7e) && (vt100_to_unicode[g - 0x41])) {
				utf = vt100_to_unicode[g - 0x41];
			}
			break;

		case 'A': /* UK, replaces # with GBP */
			if (g == '#') utf = 0x20a4;
			break;

		default:
			// no remap
			utf = (unsigned char)g;
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
	} else {
		// low ASCII
		out[0] = (char) utf;
		out[1] = 0;
	}
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
			cursor.fg |
			(cursor.bg<<4) |
			(cursor.visible ? 1<<8 : 0))
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
			if (!changeAttrs && changeColors) {
				encode2B(cell0->fg | (cell0->bg<<4), &w1);
				bufprint("\x03%c%c", w1.lsb, w1.msb);
			}
			else if (changeAttrs && !changeColors) {
				// attrs only
				encode2B(cell0->attrs, &w1);
				bufprint("\x04%c%c", w1.lsb, w1.msb);
			}
			else if (changeAttrs && changeColors) {
				// colors and attrs
				encode3B((u32) (
							 cell0->fg |
							 (cell0->bg<<4) |
							 (cell0->attrs<<8))
					, &lw1);
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
