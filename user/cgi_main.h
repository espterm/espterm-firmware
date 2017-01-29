#ifndef CGI_MAIN_H
#define CGI_MAIN_H

httpd_cgi_state tplScreen(HttpdConnData *connData, char *token, void **arg);
httpd_cgi_state tplAbout(HttpdConnData *connData, char *token, void **arg);

#endif // CGI_MAIN_H
