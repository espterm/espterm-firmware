#ifndef CGIPERSIST_H
#define CGIPERSIST_H

#include "httpd.h"

httpd_cgi_state cgiPersistWriteDefaults(HttpdConnData *connData);
httpd_cgi_state cgiPersistRestoreDefaults(HttpdConnData *connData);
httpd_cgi_state cgiPersistRestoreHard(HttpdConnData *connData);
httpd_cgi_state cgiPersistExport(HttpdConnData *connData);
httpd_cgi_state cgiPersistImport(HttpdConnData *connData);

#endif
