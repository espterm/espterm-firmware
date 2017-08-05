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
		return HTTPD_CGI_DONE;
	}

	char buff[150];

	if (streq(token, "labels_seq")) {
		screenSerializeLabelsToBuffer(buff, 150);
		httpdSend(connData, buff, -1);
	}
	else if (streq(token, "theme")) {
		sprintf(buff, "%d", termconf->theme);
		httpdSend(connData, buff, -1);
	}

	return HTTPD_CGI_DONE;
}

httpd_cgi_state ICACHE_FLASH_ATTR
cgiTermInitialImage(HttpdConnData *connData)
{
	const int bufsiz = 512;
	char buff[bufsiz];

	if (connData->conn == NULL) {
		//Connection aborted. Clean up.
		// Release data object
		screenSerializeToBuffer(NULL, 0, &connData->cgiData);
		return HTTPD_CGI_DONE;
	}

	if (connData->cgiData == NULL) {
		httpdStartResponse(connData, 200);
		httpdHeader(connData, "Content-Type", "application/octet-stream");
		httpdEndHeaders(connData);
	}

	httpd_cgi_state cont = screenSerializeToBuffer(buff, bufsiz, &connData->cgiData);
	httpdSend(connData, buff, -1);
	return cont;
}

/** "About" page */
httpd_cgi_state ICACHE_FLASH_ATTR
tplAbout(HttpdConnData *connData, char *token, void **arg)
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
