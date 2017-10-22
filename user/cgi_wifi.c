/*
Cgi/template routines for the /wifi url.
*/

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 *
 * File adapted and improved by Ondřej Hruška <ondra@ondrovo.com>
 */

#include <esp8266.h>
#include <httpdespfs.h>
#include "cgi_wifi.h"
#include "wifimgr.h"
#include "persist.h"
#include "helpers.h"
#include "config_xmacros.h"
#include "cgi_logging.h"

#define SET_REDIR_SUC "/cfg/wifi"
#define SET_REDIR_ERR SET_REDIR_SUC"?err="

/** WiFi access point data */
typedef struct {
	char ssid[32];
	char bssid[8];
	int channel;
	char rssi;
	char enc;
} ApData;

/** Scan result type */
typedef struct {
	char scanInProgress; //if 1, don't access the underlying stuff from the webpage.
	ApData **apData;
	int noAps;
} ScanResultData;

/** Static scan status storage. */
static ScanResultData cgiWifiAps;

/** Connection to AP periodic check timer */
static os_timer_t staCheckTimer;

/**
 * Calculate approximate signal strength % from RSSI
 */
int ICACHE_FLASH_ATTR rssi2perc(int rssi)
{
	int r;

	if (rssi > 200)
		r = 100;
	else if (rssi < 100)
		r = 0;
	else
		r = 100 - 2 * (200 - rssi); // approx.

	if (r > 100) r = 100;
	if (r < 0) r = 0;

	return r;
}

/**
 * Callback the code calls when a wlan ap scan is done. Basically stores the result in
 * the static cgiWifiAps struct.
 *
 * @param arg - a pointer to {struct bss_info}, which is a linked list of the found APs
 * @param status - OK if the scan succeeded
 */
void ICACHE_FLASH_ATTR wifiScanDoneCb(void *arg, STATUS status)
{
	int n;
	struct bss_info *bss_link = (struct bss_info *) arg;
	cgi_dbg("wifiScanDoneCb %d", status);
	if (status != OK) {
		cgiWifiAps.scanInProgress = 0;
		return;
	}

	// Clear prev ap data if needed.
	if (cgiWifiAps.apData != NULL) {
		for (n = 0; n < cgiWifiAps.noAps; n++) free(cgiWifiAps.apData[n]);
		free(cgiWifiAps.apData);
	}

	// Count amount of access points found.
	n = 0;
	while (bss_link != NULL) {
		bss_link = bss_link->next.stqe_next;
		n++;
	}
	// Allocate memory for access point data
	cgiWifiAps.apData = (ApData **) malloc(sizeof(ApData *) * n);
	if (cgiWifiAps.apData == NULL) {
		error("Out of memory allocating apData");
		return;
	}
	cgiWifiAps.noAps = n;
	cgi_info("Scan done: found %d APs", n);

	// Copy access point data to the static struct
	n = 0;
	bss_link = (struct bss_info *) arg;
	while (bss_link != NULL) {
		if (n >= cgiWifiAps.noAps) {
			// This means the bss_link changed under our nose. Shouldn't happen!
			// Break because otherwise we will write in unallocated memory.
			error("Huh? I have more than the allocated %d aps!", cgiWifiAps.noAps);
			break;
		}
		// Save the ap data.
		cgiWifiAps.apData[n] = (ApData *) malloc(sizeof(ApData));
		if (cgiWifiAps.apData[n] == NULL) {
			error("Can't allocate mem for ap buff.");
			cgiWifiAps.scanInProgress = 0;
			return;
		}
		cgiWifiAps.apData[n]->rssi = bss_link->rssi;
		cgiWifiAps.apData[n]->channel = bss_link->channel;
		cgiWifiAps.apData[n]->enc = bss_link->authmode;
		strncpy(cgiWifiAps.apData[n]->ssid, (char *) bss_link->ssid, 32);
		strncpy(cgiWifiAps.apData[n]->bssid, (char *) bss_link->bssid, 6);

		bss_link = bss_link->next.stqe_next;
		n++;
	}
	// We're done.
	cgiWifiAps.scanInProgress = 0;
}

/**
 * Routine to start a WiFi access point scan.
 */
static void ICACHE_FLASH_ATTR wifiStartScan(void)
{
	if (cgiWifiAps.scanInProgress) return;
	cgiWifiAps.scanInProgress = 1;
	wifi_station_scan(NULL, wifiScanDoneCb);
}

/**
 * Start a scan and return a result of an earlier scan, if available.
 * The STA is switched ON if disabled.
 */
httpd_cgi_state ICACHE_FLASH_ATTR cgiWiFiScan(HttpdConnData *connData)
{
	int pos = (int) connData->cgiData;
	int len;
	char buff[256];

	if (connData->conn == NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	// auto-turn on STA
	if ((wificonf->opmode & STATION_MODE) == 0) {
		wificonf->opmode |= STATION_MODE;
		wifimgr_apply_settings();
	}

	// 2nd and following runs of the function via MORE:
	if (!cgiWifiAps.scanInProgress && pos != 0) {
		// Fill in json code for an access point
		if (pos - 1 < cgiWifiAps.noAps) {
			int rssi = cgiWifiAps.apData[pos - 1]->rssi;

			len = sprintf(buff, "{\"essid\": \"%s\", \"bssid\": \""
				MACSTR
				"\", \"rssi\": %d, \"rssi_perc\": %d, \"enc\": %d, \"channel\": %d}%s",
						  cgiWifiAps.apData[pos - 1]->ssid,
						  MAC2STR(cgiWifiAps.apData[pos - 1]->bssid),
						  rssi,
						  rssi2perc(rssi),
						  cgiWifiAps.apData[pos - 1]->enc,
						  cgiWifiAps.apData[pos - 1]->channel,
						  (pos - 1 == cgiWifiAps.noAps - 1) ? "\n  " : ",\n   "); //<-terminator

			httpdSend(connData, buff, len);
		}
		pos++;
		if ((pos - 1) >= cgiWifiAps.noAps) {
			len = sprintf(buff, "  ]\n }\n}"); // terminate the whole object
			httpdSend(connData, buff, len);
			// Also start a new scan.
			wifiStartScan();
			return HTTPD_CGI_DONE;
		}
		else {
			connData->cgiData = (void *) pos;
			return HTTPD_CGI_MORE;
		}
	}

	// First run of the function
	httpdStartResponse(connData, 200);
	httpdHeader(connData, "Content-Type", "application/json");
	httpdEndHeaders(connData);

	if (cgiWifiAps.scanInProgress == 1) {
		// We're still scanning. Tell Javascript code that.
		len = sprintf(buff, "{\n \"result\": {\n  \"inProgress\": 1\n }\n}");
		httpdSend(connData, buff, len);
		return HTTPD_CGI_DONE;
	}
	else {
		// We have a scan result. Pass it on.
		len = sprintf(buff, "{\n \"result\": {\n  \"inProgress\": 0,\n  \"APs\": [\n   ");
		httpdSend(connData, buff, len);
		if (cgiWifiAps.apData == NULL) cgiWifiAps.noAps = 0;
		connData->cgiData = (void *) 1;
		return HTTPD_CGI_MORE;
	}
}

/**
 * Cgi to get connection status.
 *
 * This endpoint returns JSON with keys:
 * - status = 'idle', 'working' or 'fail',
 * - ip = IP address, after connection succeeds
 */
httpd_cgi_state ICACHE_FLASH_ATTR cgiWiFiConnStatus(HttpdConnData *connData)
{
	char buff[100];
	struct ip_info info;

	buff[0] = 0; // avoid unitialized read

	if (connData->conn == NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	httpdStartResponse(connData, 200);
	httpdHeader(connData, "Content-Type", "application/json");
	httpdEndHeaders(connData);

	// if bad opmode or no SSID configured, skip any checks
	if (!(wificonf->opmode & STATION_MODE) || wificonf->sta_ssid[0] == 0) {
		httpdSend(connData, "{\"status\": \"disabled\"}", -1);
		return HTTPD_CGI_DONE;
	}

	STATION_STATUS st = wifi_station_get_connect_status();
	cgi_dbg("CONN STATE = %d", st);
	switch(st) {
		case STATION_IDLE:
			sprintf(buff, "{\"status\": \"idle\"}"); // unclear when this is used
			break;

		case STATION_CONNECTING:
			sprintf(buff, "{\"status\": \"working\"}");
			break;

		case STATION_WRONG_PASSWORD:
			sprintf(buff, "{\"status\": \"fail\", \"cause\": \"WRONG_PASSWORD\"}");
			break;

		case STATION_NO_AP_FOUND:
			sprintf(buff, "{\"status\": \"fail\", \"cause\": \"AP_NOT_FOUND\"}");
			break;

		case STATION_CONNECT_FAIL:
			sprintf(buff, "{\"status\": \"fail\", \"cause\": \"CONNECTION_FAILED\"}");
			break;

		case STATION_GOT_IP:
			wifi_get_ip_info(STATION_IF, &info);
			sprintf(buff, "{\"status\": \"success\", \"ip\": \""IPSTR"\"}", GOOD_IP2STR(info.ip.addr));
			break;

		default:
			sprintf(buff, "{\"status\": \"working\", \"wtf\": \"state = %d\"}", st);
			break;

	}

	httpdSend(connData, buff, -1);
	return HTTPD_CGI_DONE;
}

/**
 * Callback for async timer
 */
static void ICACHE_FLASH_ATTR applyWifiSettingsLaterCb(void *arg)
{
	(void*)arg;
	wifimgr_apply_settings();
}

/**
 * Universal CGI endpoint to set WiFi params.
 * Note that some may cause a (delayed) restart.
 */
httpd_cgi_state ICACHE_FLASH_ATTR cgiWiFiSetParams(HttpdConnData *connData)
{
	static ETSTimer timer;

	char buff[50];
	char redir_url_buf[100]; // this is just barely enough - but it's split into two forms, so we never have error in all fields

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

	bool sta_turned_on = false;
	bool sta_ssid_pw_changed = false;

#define XSTRUCT wificonf
#define X XSET_CGI_FUNC
	XTABLE_WIFICONF
#undef X

	// ---- WiFi opmode ----

	// those are helpers, not a real prop

	if (GET_ARG("ap_enable")) {
		cgi_dbg("Enable AP: %s", buff);
		int enable = atoi(buff);

		if (enable) {
			wificonf->opmode |= SOFTAP_MODE;
		} else {
			wificonf->opmode &= ~SOFTAP_MODE;
		}
	}

	if (GET_ARG("sta_enable")) {
		cgi_dbg("Enable STA: %s", buff);
		int enable = atoi(buff);

		if (enable) {
			wificonf->opmode |= STATION_MODE;
			sta_turned_on = true;
		} else {
			wificonf->opmode &= ~STATION_MODE;
		}
	}

	(void)redir_url;

	if (redir_url_buf[strlen(SET_REDIR_ERR)] == 0) {
		// All was OK
		cgi_info("Set WiFi params - success, applying in 2000 ms");

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
		os_timer_arm(&timer, 2000, false);

		if ((sta_ssid_pw_changed || sta_turned_on)
			&& wificonf->opmode != SOFTAP_MODE
			&& wificonf->sta_ssid[0] != 0) {
			// User wants to connect

			cgi_info("User wants to connect to SSID, redirecting to ConnStatus page.");
			httpdRedirect(connData, "/cfg/wifi/connecting");
		}
		else {
			httpdRedirect(connData, SET_REDIR_SUC "?msg=Settings%20saved%20and%20applied.");
		}
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
httpd_cgi_state ICACHE_FLASH_ATTR tplWlan(HttpdConnData *connData, char *token, void **arg)
{
	char buff[PASSWORD_LEN];
	int x;
	int connectStatus;

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
	if (streq(token, "sta_enable")) {
		sprintf(buff, "%d", (wificonf->opmode & STATION_MODE) != 0);
	}
	else if (streq(token, "ap_enable")) {
		sprintf(buff, "%d", (wificonf->opmode & SOFTAP_MODE) != 0);
	}
	else if (streq(token, "sta_rssi")) {
		sprintf(buff, "%d", wifi_station_get_rssi());
	}
	else if (streq(token, "sta_active_ssid")) {
		// For display of our current SSID
		connectStatus = wifi_station_get_connect_status();
		x = wifi_get_opmode();
		if (x == SOFTAP_MODE || connectStatus != STATION_GOT_IP || wificonf->opmode == SOFTAP_MODE) {
			strcpy(buff, "");
		}
		else {
			struct station_config staconf;
			wifi_station_get_config(&staconf);
			strcpy(buff, (char *) staconf.ssid);
		}
	}
	else if (streq(token, "sta_active_ip")) {
		getStaIpAsString(buff);
	}

	tplSend(connData, buff, -1);
	return HTTPD_CGI_DONE;
}
