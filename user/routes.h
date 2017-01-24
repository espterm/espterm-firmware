#ifndef ROUTES_H
#define ROUTES_H

#include <esp8266.h>
#include <httpd.h>

extern HttpdBuiltInUrl builtInUrls[];

/** Broadcast screen state to sockets */
void screen_notifyChange();

#endif //ROUTES_H
