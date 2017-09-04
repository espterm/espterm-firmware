#include <esp8266.h>
#include <httpd.h>
#include <cgiwebsocket.h>

#include "cgi_sockets.h"
#include "uart_driver.h"
#include "screen.h"
#include "uart_buffer.h"
#include "ansi_parser.h"
#include "jstring.h"

#define LOOPBACK 0

#define SOCK_BUF_LEN 1024
static char sock_buff[SOCK_BUF_LEN];

volatile bool notify_available = true;
volatile bool notify_cooldown = false;

static ETSTimer notifyContentTim;
static ETSTimer notifyLabelsTim;
static ETSTimer notifyCooldownTim;
static ETSTimer heartbeatTim;

// we're trying to do a kind of mutex here, without the actual primitives
// this might glitch, very rarely.
// it's recommended to put some delay between setting labels and updating the screen.

/**
 * Cooldown delay is over
 * @param arg
 */
static void ICACHE_FLASH_ATTR
notifyCooldownTimCb(void *arg)
{
	notify_cooldown = false;
}

static void ICACHE_FLASH_ATTR
notifyContentTimCb(void *arg)
{
	void *data = NULL;
	int max_bl, total_bl;
	cgiWebsockMeasureBacklog(URL_WS_UPDATE, &max_bl, &total_bl);

	if (!notify_available || notify_cooldown || (max_bl > 2048)) { // do not send if we have anything significant backlogged
		// postpone a little
		TIMER_START(&notifyContentTim, notifyContentTimCb, 5, 0);
		return;
	}
	notify_available = false;

	for (int i = 0; i < 20; i++) {
		httpd_cgi_state cont = screenSerializeToBuffer(sock_buff, SOCK_BUF_LEN, &data);
		int flg = 0;
		if (cont == HTTPD_CGI_MORE) flg |= WEBSOCK_FLAG_MORE;
		if (i > 0) flg |= WEBSOCK_FLAG_CONT;
		cgiWebsockBroadcast(URL_WS_UPDATE, sock_buff, (int) strlen(sock_buff), flg);
		if (cont == HTTPD_CGI_DONE) break;
	}

	// cleanup
	screenSerializeToBuffer(NULL, SOCK_BUF_LEN, &data);

	notify_cooldown = true;
	notify_available = true;

	TIMER_START(&notifyCooldownTim, notifyCooldownTimCb, termconf->display_cooldown_ms, 0);
}

static void ICACHE_FLASH_ATTR
notifyLabelsTimCb(void *arg)
{
	if (!notify_available || notify_cooldown) {
		// postpone a little
		TIMER_START(&notifyLabelsTim, notifyLabelsTimCb, 1, 0);
		return;
	}
	notify_available = false;

	screenSerializeLabelsToBuffer(sock_buff, SOCK_BUF_LEN);
	cgiWebsockBroadcast(URL_WS_UPDATE, sock_buff, (int) strlen(sock_buff), 0);

	notify_cooldown = true;
	notify_available = true;

	TIMER_START(&notifyCooldownTim, notifyCooldownTimCb, termconf->display_cooldown_ms, 0);
}

/** Beep */
void ICACHE_FLASH_ATTR
send_beep(void)
{
	// here's some potential for a race error with the other broadcast functions :C
	cgiWebsockBroadcast(URL_WS_UPDATE, "B", 1, 0);
}


/**
 * Broadcast screen state to sockets.
 * This is a callback for the Screen module,
 * called after each visible screen modification.
 */
void ICACHE_FLASH_ATTR screen_notifyChange(ScreenNotifyChangeTopic topic)
{
	// this is not the most ideal/cleanest implementation
	// PRs are welcome for a nicer update "queue" solution
	if (termconf->display_tout_ms == 0) termconf->display_tout_ms = SCR_DEF_DISPLAY_TOUT_MS;

	// NOTE: the timers are restarted if already running

	if (topic == CHANGE_LABELS) {
		// separate timer from content change timer, to avoid losing that update
		TIMER_START(&notifyLabelsTim, notifyLabelsTimCb, termconf->display_tout_ms, 0);
	}
	else if (topic == CHANGE_CONTENT) {
		// throttle delay
		TIMER_START(&notifyContentTim, notifyContentTimCb, termconf->display_tout_ms, 0);
	}
}

void ICACHE_FLASH_ATTR sendMouseAction(char evt, int y, int x, int button, u8 mods)
{
	bool ctrl = (mods & 1) > 0;
	bool shift = (mods & 2) > 0;
	bool alt = (mods & 4) > 0;
	bool meta = (mods & 8) > 0;
	enum MTM mtm = mouse_tracking.mode;
	enum MTE mte = mouse_tracking.encoding;

	// No message on release in X10 mode
	if (mtm == MTM_X10 && button == 0) {
		return;
	}

	if (evt == 'm' && mtm != MTM_BUTTON_MOTION && mtm != MTM_ANY_MOTION) {
		return;
	}

	if (evt == 'm' && mtm == MTM_BUTTON_MOTION && button == 0) {
		return;
	}

	int eventcode = 0;

	if (mtm == MTM_X10) {
		eventcode = button;
	}
	else {
		if (button == 0 || (evt == 'r' && mte != MTE_SGR)) eventcode = 3; // release
		else if (button == 1) eventcode = 0;
		else if (button == 2) eventcode = 1;
		else if (button == 3) eventcode = 2;
		else if (button == 4) eventcode = 64;
		else if (button == 5) eventcode = 65;

		if (shift) eventcode |= 4;
		if (alt || meta) eventcode |= 8;
		if (ctrl) eventcode |= 16;

		if (mtm == MTM_BUTTON_MOTION || mtm == MTM_ANY_MOTION) {
			if (evt == 'm') {
				eventcode |= 32;
			}
		}
	}

	// Encode
	char buf[20];
	buf[0] = 0;
	if (mte == MTE_SIMPLE || mte == MTE_UTF8) {
		// strictly, for UTF8 this will break if any coord is over 127,
		// but that is unlikely due to screen size limitations in ESPTerm
		sprintf(buf, "\x1b[M%c%c%c", (u8)(32+eventcode), (u8)(32+x), (u8)(32+y));
	}
	else if (mte == MTE_SGR) {
		sprintf(buf, "\x1b[%d;%d;%d%c", eventcode, x, y, evt == 'p' ? 'M' : 'm');
	}
	else if (mte == MTE_URXVT) {
		sprintf(buf, "\x1b[%d;%d;%dM", (u8)(32+eventcode), (u8)(32+x), (u8)(32+y));
	}

	UART_SendAsync(buf, -1);
}

/** Socket received a message */
void ICACHE_FLASH_ATTR updateSockRx(Websock *ws, char *data, int len, int flags)
{
	// Add terminator if missing (seems to randomly happen)
	data[len] = 0;

	ws_dbg("Sock RX str: %s, len %d", data, len);

	int y, x, m, b;
	u8 btnNum;

	char c = data[0];
	switch (c) {
		case 's':
		// pass string verbatim
#if LOOPBACK
		for(int i=4;i<strlen(data); i++) {
			ansi_parser(data[i]);
		}
#else
			UART_SendAsync(data+1, -1);
#endif
			break;
		case 'b':
			// action button press
			btnNum = (u8) (data[1]);
			if (btnNum > 0 && btnNum < 10) {
				UART_SendAsync((const char *) &btnNum, 1); // TODO this is where we use user-configured codes
			}
			break;
		case 'm':
		case 'p':
		case 'r':
			if (mouse_tracking.mode == MTM_NONE) break; // no need to parse, not enabled

			// mouse move
			y = parse2B(data+1); // row, 0-based
			x = parse2B(data+3); // column, 0-based
			b = parse2B(data+5); // mouse button, 0 = none, 1-5 = button number
			m = parse2B(data+7); // modifier keys held

			sendMouseAction(c,y,x,b,m);
			break;
		default:
			ws_warn("Bad command.");
	}
}

void ICACHE_FLASH_ATTR heartbeatTimCb(void *unused)
{
	if (notify_available) {
		// Heartbeat packet - indicate we're still connected
		// JS reloads the page if heartbeat is lost for a couple seconds
		cgiWebsockBroadcast(URL_WS_UPDATE, ".", 1, 0);
	}
}

/** Socket connected for updates */
void ICACHE_FLASH_ATTR updateSockConnect(Websock *ws)
{
	ws_info("Socket connected to "URL_WS_UPDATE);
	ws->recvCb = updateSockRx;

	TIMER_START(&heartbeatTim, heartbeatTimCb, 1000, 1);
}
