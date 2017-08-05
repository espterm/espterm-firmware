#ifndef CGI_SOCKETS_H
#define CGI_SOCKETS_H

#define URL_WS_UPDATE "/term/update.ws"

#include "screen.h"

/** Update websocket connect callback */
void updateSockConnect(Websock *ws);
void screen_notifyChange(ScreenNotifyChangeTopic topic);

#endif //CGI_SOCKETS_H
