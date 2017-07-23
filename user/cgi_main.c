#include <esp8266.h>
#include <httpd.h>
#include <esp_sdk_ver.h>

#include "cgi_main.h"
#include "screen.h"
#include "user_main.h"
#include "helpers.h"

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

	if (streq(token, "term_title")) {
		httpdSend(connData, termconf->title, -1);
	}
	else if (streq(token, "btn1")) {
		httpdSend(connData, termconf->btn1, -1);
	}
	else if (streq(token, "btn2")) {
		httpdSend(connData, termconf->btn2, -1);
	}
	else if (streq(token, "btn3")) {
		httpdSend(connData, termconf->btn3, -1);
	}
	else if (streq(token, "btn4")) {
		httpdSend(connData, termconf->btn4, -1);
	}
	else if (streq(token, "btn5")) {
		httpdSend(connData, termconf->btn5, -1);
	}
	else if (streq(token, "default_bg")) {
		sprintf(buff, "%d", termconf->default_bg);
		httpdSend(connData, buff, -1);
	}
	else if (streq(token, "default_fg")) {
		sprintf(buff, "%d", termconf->default_fg);
		httpdSend(connData, buff, -1);
	}
	else if (streq(token, "screenData")) {
		httpd_cgi_state cont = screenSerializeToBuffer(buff, bufsiz, arg);
		httpdSend(connData, buff, -1);
		return cont;
	}

	return HTTPD_CGI_DONE;
}

/** "About" page */
httpd_cgi_state ICACHE_FLASH_ATTR tplAbout(HttpdConnData *connData, char *token, void **arg)
{
	if (token == NULL) return HTTPD_CGI_DONE;

	if (streq(token, "vers_fw")) {
		httpdSend(connData, FIRMWARE_VERSION, -1);
	}
	else if (streq(token, "date")) {
		httpdSend(connData, __DATE__, -1);
	}
	else if (streq(token, "time")) {
		httpdSend(connData, __TIME__, -1);
	}
	else if (streq(token, "vers_httpd")) {
		httpdSend(connData, HTTPDVER, -1);
	}
	else if (streq(token, "vers_sdk")) {
		httpdSend(connData, STR(ESP_SDK_VERSION), -1);
	}
	else if (streq(token, "githubrepo")) {
		httpdSend(connData, TERMINAL_GITHUB_REPO, -1);
	}

	return HTTPD_CGI_DONE;
}
