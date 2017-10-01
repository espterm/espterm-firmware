#include <esp8266.h>
#include <httpd.h>
#include <cgiwebsocket.h>

#include "cgi_sockets.h"
#include "screen.h"
#include "uart_buffer.h"
#include "ansi_parser.h"
#include "jstring.h"
#include "uart_driver.h"

// Heartbeat interval in ms
#define HB_TIME 1000

// Buffer size (sent in one go)
// Must be less than httpd sendbuf
#define SOCK_BUF_LEN 2000

volatile ScreenNotifyTopics pendingBroadcastTopics = 0;

// flags for screen update timeouts
volatile bool notify_available = true;
volatile bool notify_cooldown = false;
volatile bool notify_scheduled = false;

/** True if we sent XOFF to browser to stop uploading,
 * and we have to tell it we're ready again */
volatile bool browser_wants_xon = false;

static ETSTimer updateNotifyTim;
static ETSTimer notifyCooldownTim;
static ETSTimer heartbeatTim;

volatile int term_active_clients = 0;

// we're trying to do a kind of mutex here, without the actual primitives
// this might glitch, very rarely.
// it's recommended to put some delay between setting labels and updating the screen.

static void resetHeartbeatTimer(void);

/**
 * Cooldown delay is over
 * @param arg
 */
static void ICACHE_FLASH_ATTR
notifyCooldownTimCb(void *arg)
{
	notify_cooldown = false;
}

/**
 * Tell browser we have new content
 * @param arg
 */
static void ICACHE_FLASH_ATTR
updateNotify_do(Websock *ws, ScreenNotifyTopics topics)
{
	void *data = NULL;
	char sock_buff[SOCK_BUF_LEN];

	notify_available = false;

	for (int i = 0; i < 20; i++) {
		if (! ws) {
			// broadcast
			topics = pendingBroadcastTopics;
			pendingBroadcastTopics = 0;
		}
		httpd_cgi_state cont = screenSerializeToBuffer(sock_buff, SOCK_BUF_LEN, topics, &data);

		int flg = WEBSOCK_FLAG_BIN;
		if (cont == HTTPD_CGI_MORE) flg |= WEBSOCK_FLAG_MORE;
		if (i > 0) flg |= WEBSOCK_FLAG_CONT;
		if (ws) {
			cgiWebsocketSend(ws, sock_buff, (int) strlen(sock_buff), flg);
		} else {
			cgiWebsockBroadcast(URL_WS_UPDATE, sock_buff, (int) strlen(sock_buff), flg);
			resetHeartbeatTimer();
		}
		if (cont == HTTPD_CGI_DONE) break;

		system_soft_wdt_feed();
	}

	// cleanup
	screenSerializeToBuffer(NULL, 0, 0, &data);

	notify_available = true;
}

/**
 * Tell browser we have new content
 * @param arg
 */
static void ICACHE_FLASH_ATTR
updateNotifyCb(void *arg)
{
	int max_bl, total_bl;
	cgiWebsockMeasureBacklog(URL_WS_UPDATE, &max_bl, &total_bl);

	inp_dbg("Notify broadcast +%02Xh?", pendingBroadcastTopics);

	if (!notify_available || notify_cooldown || (max_bl > 2048)) { // do not send if we have anything significant backlogged
		// postpone a little
		TIMER_START(&updateNotifyTim, updateNotifyCb, 4, 0);
		inp_dbg("postpone notify; avail? %d coold? %d maxbl? %d", notify_available, notify_cooldown, max_bl);
		return;
	}

	updateNotify_do(arg, 0);

	notify_scheduled = false;
	notify_cooldown = true;

	TIMER_START(&notifyCooldownTim, notifyCooldownTimCb, termconf->display_cooldown_ms, 0);
}


/** Beep */
void ICACHE_FLASH_ATTR
send_beep(void)
{
	if (term_active_clients == 0) return;
	screen_notifyChange(TOPIC_BELL); // XXX has latency, maybe better to send directly
}


/** Pop-up notification (from iTerm2) */
void ICACHE_FLASH_ATTR
notify_growl(char *msg)
{
	if (term_active_clients == 0) return;

	// here's some potential for a race error with the other broadcast functions
	// - we assume app won't send notifications in the middle of updating content
	cgiWebsockBroadcast(URL_WS_UPDATE, msg, (int) strlen(msg), WEBSOCK_FLAG_BIN);
	resetHeartbeatTimer();
}


/**
 * Broadcast screen state to sockets.
 * This is a callback for the Screen module,
 * called after each visible screen modification.
 */
void ICACHE_FLASH_ATTR screen_notifyChange(ScreenNotifyTopics topics)
{
	if (term_active_clients == 0) return;

	inp_dbg("Notify +%02Xh", topics);

	pendingBroadcastTopics |= topics;

	int time = termconf->display_tout_ms;
	if (time == 0 && notify_scheduled) return; // do not reset the timer if already scheduled

	notify_scheduled = true;
	// NOTE: the timer is restarted if already running
	TIMER_START(&updateNotifyTim, updateNotifyCb, time, 0); // note - this adds latency to beep
}

/**
 * mouse event rx
 * @param evt - event type: p, r, m
 * @param y - coord 0-based
 * @param x - coord 0-based
 * @param button - which button, 1-based. 0=none
 * @param mods - modifier keys bitmap: meta|alt|shift|ctrl
 */
static void ICACHE_FLASH_ATTR sendMouseAction(char evt, int y, int x, int button, u8 mods)
{
	// one-based
	x++;
	y++;

	bool ctrl = (mods & 1) > 0;
	bool shift = (mods & 2) > 0;
	bool alt = (mods & 4) > 0;
	bool meta = (mods & 8) > 0;
	enum MTM mtm = mouse_tracking.mode;
	enum MTE mte = mouse_tracking.encoding;

	// No message on release in X10 mode
	if (mtm == MTM_X10 && (button == 0 || evt == 'r')) {
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
		eventcode = button-1;
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
		sprintf(buf, "\x1b[<%d;%d;%d%c", eventcode, x, y, (evt == 'p'||(evt=='m'&&button>0)) ? 'M' : 'm');
	}
	else if (mte == MTE_URXVT) {
		sprintf(buf, "\x1b[%d;%d;%dM", (u8)(32+eventcode), (u8)(x), (u8)(y));
	}

	UART_SendAsync(buf, -1);
}

/** Socket received a message */
static void ICACHE_FLASH_ATTR updateSockRx(Websock *ws, char *data, int len, int flags)
{
	// Add terminator if missing (seems to randomly happen)
	data[len] = 0;

	inp_dbg("Sock RX str: %s, len %d", data, len);

	int y, x, m, b;
	u8 btnNum;

	char c = data[0];
	switch (c) {
		case 's':
			// pass string verbatim
			if (termconf_live.loopback) {
				for (int i = 1; i < len; i++) {
					ansi_parser(data[i]);
				}
			}
			UART_SendAsync(data+1, -1);

			// TODO base this on the actual buffer empty space, not rx chunk size
			if ((UART_AsyncTxGetEmptySpace() < 256) && !browser_wants_xon) {
				UART_WriteChar(UART1, '-', 100);
				cgiWebsockBroadcast(URL_WS_UPDATE, "-", 1, 0);
				browser_wants_xon = true;

				system_soft_wdt_feed();
			}
			break;

		case 'b':
			// action button press
			btnNum = (u8) (data[1]);
			if (btnNum > 0 && btnNum < 10) {
				UART_SendAsync(termconf_live.btn_msg[btnNum-1], -1);
			}
			break;

		case 'i':
			// requests initial load
			inp_dbg("Client requests initial load");
			updateNotify_do(ws, TOPIC_INITIAL|TOPIC_FLAG_NOCLEAN);
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
			inp_warn("Bad command.");
	}
}

/** Send a heartbeat msg */
static void ICACHE_FLASH_ATTR heartbeatTimCb(void *unused)
{
	if (term_active_clients > 0) {
		if (notify_available) {
			inp_dbg(".");

			// Heartbeat packet - indicate we're still connected
			// JS reloads the page if heartbeat is lost for a couple seconds
			cgiWebsockBroadcast(URL_WS_UPDATE, ".", 1, 0);

			// schedule next tick
			TIMER_START(&heartbeatTim, heartbeatTimCb, HB_TIME, 0);
		} else {
			// postpone...
			TIMER_START(&heartbeatTim, heartbeatTimCb, 10, 0);
			inp_dbg("postpone heartbeat");
		}
	}
}

static void ICACHE_FLASH_ATTR resetHeartbeatTimer(void)
{
	TIMER_START(&heartbeatTim, heartbeatTimCb, HB_TIME, 0);
}


static void ICACHE_FLASH_ATTR closeSockCb(Websock *ws)
{
	term_active_clients--;
	if (term_active_clients <= 0) {
		term_active_clients = 0;

		if (mouse_tracking.focus_tracking) {
			UART_SendAsync("\x1b[O", 3);
		}

		// stop the timer
		os_timer_disarm(&heartbeatTim);
	}
}

/** Socket connected for updates */
void ICACHE_FLASH_ATTR updateSockConnect(Websock *ws)
{
	inp_info("Socket connected to "URL_WS_UPDATE);
	ws->recvCb = updateSockRx;
	ws->closeCb = closeSockCb;

	if (term_active_clients == 0) {
		if (mouse_tracking.focus_tracking) {
			UART_SendAsync("\x1b[I", 3);
		}

		resetHeartbeatTimer();
	}

	term_active_clients++;
}

ETSTimer xonTim;

static void ICACHE_FLASH_ATTR notify_empty_txbuf_cb(void *unused)
{
	UART_WriteChar(UART1, '+', 100);
	cgiWebsockBroadcast(URL_WS_UPDATE, "+", 1, 0);
	resetHeartbeatTimer();
	browser_wants_xon = false;
}

void notify_empty_txbuf(void)
{
	if (browser_wants_xon) {
		TIMER_START(&xonTim, notify_empty_txbuf_cb, 1, 0);
	}
}
