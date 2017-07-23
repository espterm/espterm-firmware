#ifndef CGINET_H
#define CGINET_H

#include "httpd.h"

httpd_cgi_state cgiNetworkSetParams(HttpdConnData *connData);
httpd_cgi_state tplNetwork(HttpdConnData *connData, char *token, void **arg);

#endif
