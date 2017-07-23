#ifndef CGIWIFI_H
#define CGIWIFI_H

#include "httpd.h"
#include "helpers.h"

httpd_cgi_state cgiWiFiScan(HttpdConnData *connData);

httpd_cgi_state cgiWiFiSetParams(HttpdConnData *connData);
httpd_cgi_state tplWlan(HttpdConnData *connData, char *token, void **arg);
httpd_cgi_state cgiWiFiConnStatus(HttpdConnData *connData);

#endif
