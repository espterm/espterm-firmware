#ifndef CGI_H
#define CGI_H

#include "httpd.h"

httpd_cgi_state cgiLed(HttpdConnData *connData);
httpd_cgi_state tplLed(HttpdConnData *connData, char *token, void **arg);
httpd_cgi_state tplCounter(HttpdConnData *connData, char *token, void **arg);

#endif
