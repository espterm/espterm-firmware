#ifndef CGI_PING_H
#define CGI_PING_H

#include <esp8266.h>
#include <httpd.h>

httpd_cgi_state cgiPing(HttpdConnData *connData);
httpd_cgi_state cgiResetDevice(HttpdConnData *connData);
httpd_cgi_state cgiSystemCfgSetParams(HttpdConnData *connData);
httpd_cgi_state tplSystemCfg(HttpdConnData *connData, char *token, void **arg);

#endif // CGI_PING_H
