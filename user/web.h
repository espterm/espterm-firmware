#ifndef WEB_H
#define WEB_H

#include <esp8266.h>
#include <httpd.h>

extern HttpdBuiltInUrl builtInUrls[];

/** Broadcast screen state to sockets */
void screen_notifyChange();

#endif //WEB_H
