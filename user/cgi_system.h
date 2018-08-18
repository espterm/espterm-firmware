#ifndef CGI_PING_H
#define CGI_PING_H

#include <esp8266.h>
#include <httpd.h>

httpd_cgi_state cgiPing(HttpdConnData *connData);
httpd_cgi_state cgiResetDevice(HttpdConnData *connData);
httpd_cgi_state cgiSystemCfgSetParams(HttpdConnData *connData);
httpd_cgi_state tplSystemCfg(HttpdConnData *connData, char *token, void **arg);
httpd_cgi_state cgiResetScreen(HttpdConnData *connData);
httpd_cgi_state cgiGPIO(HttpdConnData *connData);

#endif // CGI_PING_H
