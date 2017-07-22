/*
Cgi/template routines for configuring non-wifi settings
*/

#include <esp8266.h>
#include "cgi_wifi.h"
#include "wifimgr.h"
#include "persist.h"

// strcpy that adds 0 at the end of the buffer. Returns void.
#define strncpy_safe(dst, src, n) do { strncpy((char *)(dst), (char *)(src), (n)); (dst)[(n)-1]=0; } while (0)

/**
 * Universal CGI endpoint to set WiFi params.
 * Note that some may cause a (delayed) restart.
 */
httpd_cgi_state ICACHE_FLASH_ATTR cgiAppCfgSet(HttpdConnData *connData)
{
	static ETSTimer timer;

	char buff[50];

#define REDIR_BASE_URL "/wifi?err="

	char redir_url_buf[300];
	char *redir_url = redir_url_buf;
	redir_url += sprintf(redir_url, REDIR_BASE_URL);
	// we'll test if anything was printed by looking for \0 in failed_keys_buf

	if (connData->conn == NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

#define GET_ARG(key) (httpdFindArg(connData->getArgs, key, buff, sizeof(buff)) > 0)

	// TODO
	if (GET_ARG("opmode")) {
		dbg("Setting WiFi opmode to: %s", buff);
		int mode = atoi(buff);
		if (mode > NULL_MODE && mode < MAX_MODE) {
			wificonf->opmode = (WIFI_MODE) mode;
		} else {
			warn("Bad opmode value \"%s\"", buff);
			redir_url += sprintf(redir_url, "opmode,");
		}
	}

	if (redir_url_buf[strlen(REDIR_BASE_URL)] == 0) {
		// All was OK
		info("Set WiFi params - success, applying in 1000 ms");

		// Settings are applied only if all was OK
		//
		// This is so that options that consist of multiple keys sent together are not applied
		// only partially if set wrong, which could lead to eg. user losing access and having
		// to reset to defaults.
		persist_store();

		// Delayed settings apply, so the response page has a chance to load.
		// If user connects via the Station IF, they may not even notice the connection reset.
		os_timer_disarm(&timer);
		os_timer_setfn(&timer, applyWifiSettingsLaterCb, NULL);
		os_timer_arm(&timer, 1000, false);

		httpdRedirect(connData, "/wifi");
	} else {
		warn("Some WiFi settings did not validate, asking for correction");
		// Some errors, appended to the URL as ?err=
		httpdRedirect(connData, redir_url_buf);
	}
	return HTTPD_CGI_DONE;
}


//Template code for the WLAN page.
httpd_cgi_state ICACHE_FLASH_ATTR tplAppCfg(HttpdConnData *connData, char *token, void **arg)
{
	char buff[100];
	int x;
	int connectStatus;

	if (token == NULL) {
		// We're done
		return HTTPD_CGI_DONE;
	}

	strcpy(buff, ""); // fallback

	// TODO
	if (streq(token, "opmode_name")) {
		strcpy(buff, opmode2str(wificonf->opmode));
	}

	httpdSend(connData, buff, -1);
	return HTTPD_CGI_DONE;
}
