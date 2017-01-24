#ifndef CGI_PING_H
#define CGI_PING_H

#include <esp8266.h>
#include <httpd.h>

// this is used by the UI to check if server is already restarted and working again.

httpd_cgi_state cgiPing(HttpdConnData *connData);

#endif // CGI_PING_H
