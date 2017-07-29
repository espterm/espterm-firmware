#ifndef CGIAPPCFG_H
#define CGIAPPCFG_H

#include "httpd.h"

httpd_cgi_state cgiTermCfgSetParams(HttpdConnData *connData);
httpd_cgi_state tplTermCfg(HttpdConnData *connData, char *token, void **arg);

#endif
