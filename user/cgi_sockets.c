#include <esp8266.h>
#include <httpd.h>
#include <cgiwebsocket.h>

#include "cgi_sockets.h"
#include "uart_driver.h"
#include "screen.h"

/**
 * Broadcast screen state to sockets
 */
void ICACHE_FLASH_ATTR screen_notifyChange()
{
	// TODO cooldown / buffering to reduce nr of such events

	void *data = NULL;

	const int bufsiz = 512;
	char buff[bufsiz];
	for (int i = 0; i < 20; i++) {
		httpd_cgi_state cont = screenSerializeToBuffer(buff, bufsiz, &data);
		int flg = 0;
		if (cont == HTTPD_CGI_MORE) flg |= WEBSOCK_FLAG_MORE;
		if (i > 0) flg |= WEBSOCK_FLAG_CONT;
		cgiWebsockBroadcast(URL_WS_UPDATE, buff, (int) strlen(buff), flg);
		if (cont == HTTPD_CGI_DONE) break;
	}
}

/** Socket received a message */
void ICACHE_FLASH_ATTR myWebsocketRecv(Websock *ws, char *data, int len, int flags)
{
	dbg("Sock RX str: %s, len %d", data, len);

	if (strstarts(data, "STR:")) {
		// pass string verbatim
		UART_WriteString(UART0, data+4, UART_TIMEOUT_US);
	}
	else if (strstarts(data, "BTN:")) {
		// send button as low ASCII value 1-9
		int btnNum = data[4] - '0';
		if (btnNum > 0 && btnNum < 10) {
			UART_WriteChar(UART0, (unsigned char)btnNum, UART_TIMEOUT_US);
		}
	}
	else if (strstarts(data, "TAP:")) {
		// TODO
		warn("TODO mouse input handling not implemented!");
	}
	else {
		warn("Bad command.");
	}
}

/** Socket connected for updates */
void ICACHE_FLASH_ATTR myWebsocketConnect(Websock *ws)
{
	info("Socket connected to "URL_WS_UPDATE);
	ws->recvCb = myWebsocketRecv;
}
