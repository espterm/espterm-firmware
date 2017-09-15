#ifndef CGI_SOCKETS_H
#define CGI_SOCKETS_H

#define URL_WS_UPDATE "/term/update.ws"

#include <cgiwebsocket.h>

/** Update websocket connect callback */
void updateSockConnect(Websock *ws);

void notify_empty_txbuf(void);

void send_beep(void);

/** open pop-up notification */
void notify_growl(char *msg);

// defined in the makefile
#if DEBUG_INPUT
#define inp_warn warn
#define inp_dbg dbg
#define inp_info info
#else
#define inp_warn(...)
#define inp_dbg(...)
#define inp_info(...)
#endif

#endif //CGI_SOCKETS_H
