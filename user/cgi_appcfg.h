#ifndef CGIAPPCFG_H
#define CGIAPPCFG_H

#include "httpd.h"

httpd_cgi_state cgiAppCfgSet(HttpdConnData *connData);
httpd_cgi_state tplAppCfg(HttpdConnData *connData, char *token, void **arg);

#endif