/*
configuring the network settings
*/

#include <esp8266.h>
#include <httpdespfs.h>
#include "cgi_network.h"
#include "wifimgr.h"
#include "persist.h"
#include "helpers.h"
#include "cgi_logging.h"
#include "config_xmacros.h"

#define SET_REDIR_SUC "/cfg/network"
#define SET_REDIR_ERR SET_REDIR_SUC"?err="

/**
 * Callback for async timer
 */
static void ICACHE_FLASH_ATTR applyNetSettingsLaterCb(void *arg)
{
	wifimgr_apply_settings();
}

/**
 * Universal CGI endpoint to set network params.
 * Those affect DHCP etc, may cause a disconnection.
 */
httpd_cgi_state ICACHE_FLASH_ATTR cgiNetworkSetParams(HttpdConnData *connData)
{
	static ETSTimer timer;

	char buff[20];
	char redir_url_buf[100];

	char *redir_url = redir_url_buf;
	redir_url += sprintf(redir_url, SET_REDIR_ERR);
	// we'll test if anything was printed by looking for \0 in failed_keys_buf

	if (connData->conn == NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	WiFiConfigBundle *wificonf_backup = malloc(sizeof(WiFiConfigBundle));
	WiFiConfChangeFlags *wcf_backup = malloc(sizeof(WiFiConfChangeFlags));
	memcpy(wificonf_backup, wificonf, sizeof(WiFiConfigBundle));
	memcpy(wcf_backup, &wifi_change_flags, sizeof(WiFiConfChangeFlags));

	// ---- AP DHCP server lease time ----

#define XSTRUCT wificonf
#define X XSET_CGI_FUNC
	XTABLE_WIFICONF
#undef X

	(void) redir_url;

	if (redir_url_buf[strlen(SET_REDIR_ERR)] == 0) {
		// All was OK
		cgi_info("Set network params - success, applying in 1000 ms");

		// Settings are applied only if all was OK
		persist_store();

		// Delayed settings apply, so the response page has a chance to load.
		// If user connects via the Station IF, they may not even notice the connection reset.
		os_timer_disarm(&timer);
		os_timer_setfn(&timer, applyNetSettingsLaterCb, NULL);
		os_timer_arm(&timer, 1000, false);

		httpdRedirect(connData, SET_REDIR_SUC "?msg=Settings%20saved%20and%20applied.");
	} else {
		cgi_warn("Some WiFi settings did not validate, asking for correction");

		memcpy(wificonf, wificonf_backup, sizeof(WiFiConfigBundle));
		memcpy(&wifi_change_flags, wcf_backup, sizeof(WiFiConfChangeFlags));

		// Some errors, appended to the URL as ?err=
		httpdRedirect(connData, redir_url_buf);
	}

	free(wificonf_backup);
	free(wcf_backup);
	return HTTPD_CGI_DONE;
}


//Template code for the WLAN page.
httpd_cgi_state ICACHE_FLASH_ATTR tplNetwork(HttpdConnData *connData, char *token, void **arg)
{
	char buff[64];
	u8 mac[6];

	if (token == NULL) {
		// We're done
		return HTTPD_CGI_DONE;
	}

	strcpy(buff, ""); // fallback

#define XSTRUCT wificonf
#define X XGET_CGI_FUNC
	XTABLE_WIFICONF
#undef X

	// non-config
	if (streq(token, "sta_mac")) {
		wifi_get_macaddr(STATION_IF, mac);
		sprintf(buff, MACSTR, MAC2STR(mac));
	}
	else if (streq(token, "ap_mac")) {
		wifi_get_macaddr(SOFTAP_IF, mac);
		sprintf(buff, MACSTR, MAC2STR(mac));
	}

	tplSend(connData, buff, -1);
	return HTTPD_CGI_DONE;
}
