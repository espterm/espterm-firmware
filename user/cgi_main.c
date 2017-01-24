#include <esp8266.h>
#include <httpd.h>

#include "cgi_main.h"
#include "screen.h"

/**
 * Main page template substitution
 *
 * @param connData
 * @param token
 * @param arg
 * @return
 */
httpd_cgi_state ICACHE_FLASH_ATTR tplScreen(HttpdConnData *connData, char *token, void **arg)
{
	if (token == NULL) {
		// Release data object
		screenSerializeToBuffer(NULL, 0, arg);
		return HTTPD_CGI_DONE;
	}

	const int bufsiz = 512;
	char buff[bufsiz];

	if (streq(token, "screenData")) {
		httpd_cgi_state cont = screenSerializeToBuffer(buff, bufsiz, arg);
		httpdSend(connData, buff, -1);
		return cont;
	}

	return HTTPD_CGI_DONE;
}

