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

// TODO convert to work with WiFi Manager
// TODO make changes write to wificonf and apply when a different CGI is run (/wifi/apply or something)
// TODO (connection will trigger this immediately, with some delayto show the connecting page. Then polling cna proceed as usual)

#include <esp8266.h>
#include "cgi_wifi.h"
#include "wifimgr.h"
#include "persist.h"

// strcpy that adds 0 at the end of the buffer. Returns void.
#define strncpy_safe(dst, src, n) do { strncpy((char *)(dst), (char *)(src), (n)); dst[(n)-1]=0; } while (0)

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

/** Progress of connection to AP enum */
typedef enum {
	CONNTRY_IDLE = 0,
	CONNTRY_WORKING = 1,
	CONNTRY_SUCCESS = 2,
	CONNTRY_FAIL = 3,
} ConnTry;

/** Connection result var */
static ConnTry connTryStatus = CONNTRY_IDLE;

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
 * Convert Auth type to string
 */
const ICACHE_FLASH_ATTR char *auth2str(AUTH_MODE auth)
{
	switch (auth) {
		case AUTH_OPEN:
			return "Open";
		case AUTH_WEP:
			return "WEP";
		case AUTH_WPA_PSK:
			return "WPA";
		case AUTH_WPA2_PSK:
			return "WPA2";
		case AUTH_WPA_WPA2_PSK:
			return "WPA/WPA2";
		default:
			return "Unknown";
	}
}

/**
 * Convert WiFi opmode to string
 */
const ICACHE_FLASH_ATTR char *opmode2str(WIFI_MODE opmode)
{
	switch (opmode) {
		case NULL_MODE:
			return "Disabled";
		case STATION_MODE:
			return "Client";
		case SOFTAP_MODE:
			return "AP only";
		case STATIONAP_MODE:
			return "Client+AP";
		default:
			return "Unknown";
	}
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
	dbg("wifiScanDoneCb %d", status);
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
	info("Scan done: found %d APs", n);

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
 * This CGI is called from the bit of AJAX-code in wifi.tpl. It will initiate a
 * scan for access points and if available will return the result of an earlier scan.
 * The result is embedded in a bit of JSON parsed by the javascript in wifi.tpl.
 */
httpd_cgi_state ICACHE_FLASH_ATTR cgiWiFiScan(HttpdConnData *connData)
{
	int pos = (int) connData->cgiData;
	int len;
	char buff[256];

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
 * This routine is ran some time after a connection attempt to an access point. If
 * the connect succeeds, this gets the module in STA-only mode.
 */
static void ICACHE_FLASH_ATTR staCheckConnStatus(void *arg)
{
	int x = wifi_station_get_connect_status();
	if (x == STATION_GOT_IP) {
		info("Connected to AP.");
		connTryStatus = CONNTRY_SUCCESS;

		// This would enter STA only mode, but that kills the browser page if using STA+AP.
		// Instead we stay in the current mode and let the user switch manually.

		//wifi_set_opmode(STATION_MODE);
		//system_restart();
	}
	else {
		connTryStatus = CONNTRY_FAIL;
		error("Connection failed.");
	}
}

/**
 * Delayed connect callback
 */
static void ICACHE_FLASH_ATTR cgiWiFiConnect_do(void *arg)
{
	int x;
	struct station_config cfg;

	dbg("Try to connect to AP...");

	strncpy_safe(cfg.password, wificonf->sta_password, PASSWORD_LEN);
	strncpy_safe(cfg.ssid, wificonf->sta_ssid, SSID_LEN);
	cfg.bssid_set = 0;

	wifi_station_disconnect();
	wifi_station_set_config(&cfg);
	wifi_station_connect();

	x = wifi_get_opmode();
	connTryStatus = CONNTRY_WORKING;
	// Assumption:
	// if we're in station mode, no need to check: the browser will be disconnected
	// and the user finds out whether it succeeded or not by checking if they can connect
	if (x != STATION_MODE) {
		os_timer_disarm(&staCheckTimer);
		os_timer_setfn(&staCheckTimer, staCheckConnStatus, NULL);
		os_timer_arm(&staCheckTimer, 15000, 0); //time out after 15 secs of trying to connect
	}
}

/**
 * This cgi uses the routines above to connect to a specific access point with the
 * given ESSID using the given password.
 *
 * Args:
 * - essid = SSID to connect to
 * - passwd = password to connect with
 */
httpd_cgi_state ICACHE_FLASH_ATTR cgiWiFiConnect(HttpdConnData *connData)
{
	char ssid[100];
	char password[100];
	static os_timer_t reassTimer;

	if (connData->conn == NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	int ssilen = httpdFindArg(connData->post->buff, "sta_ssid", ssid, sizeof(ssid));
	int passlen = httpdFindArg(connData->post->buff, "sta_password", password, sizeof(password));

	if (ssilen == -1 || passlen == -1) {
		error("Did not receive the required arguments!");
		httpdRedirect(connData, "/wifi");
	}
	else {
		strncpy_safe(wificonf->sta_ssid, ssid, SSID_LEN);
		strncpy_safe(wificonf->sta_password, password, PASSWORD_LEN);
		info("Try to connect to AP \"%s\" pw \"%s\".", ssid, password);

		//Schedule disconnect/connect
		os_timer_disarm(&reassTimer);
		os_timer_setfn(&reassTimer, cgiWiFiConnect_do, NULL);
		// redirect & start connecting a little bit later
		os_timer_arm(&reassTimer, 1000, 0); // was 500, increased so the connecting page has time to load

		connTryStatus = CONNTRY_IDLE;
		httpdRedirect(connData, "/wifi/connecting"); // this page is meant to show progress
	}
	return HTTPD_CGI_DONE;
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
	int len;
	struct ip_info info;
	int st = wifi_station_get_connect_status();

	httpdStartResponse(connData, 200);
	httpdHeader(connData, "Content-Type", "application/json");
	httpdEndHeaders(connData);

	if (connTryStatus == CONNTRY_IDLE) {
		len = sprintf(buff, "{\"status\": \"idle\"}");
	}
	else if (connTryStatus == CONNTRY_WORKING || connTryStatus == CONNTRY_SUCCESS) {
		if (st == STATION_GOT_IP) {
			wifi_get_ip_info(STATION_IF, &info);
			len = sprintf(buff, "{\"status\": \"success\", \"ip\": \""
				IPSTR
				"\"}", GOOD_IP2STR(info.ip.addr));
			os_timer_disarm(&staCheckTimer);
			os_timer_setfn(&staCheckTimer, staCheckConnStatus, NULL);
			os_timer_arm(&staCheckTimer, 1000, 0);
		} else {
			len = sprintf(buff, "{\"status\": \"working\"}");
		}
	}
	else {
		len = sprintf(buff, "{\"status\": \"fail\"}");
	}

	httpdSend(connData, buff, len);
	return HTTPD_CGI_DONE;
}

/** reset_later() timer */

/**
 * Callback for async timer
 */
static void ICACHE_FLASH_ATTR applyWifiSettingsLaterCb(void *arg)
{
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

	char redir_url_buf[300];
	char *redir_url = redir_url_buf;
	redir_url += sprintf(redir_url, "/wifi?err=");
	// we'll test if anything was printed by looking for \0 in failed_keys_buf

	if (connData->conn == NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

#define GET_ARG(key) (httpdFindArg(connData->getArgs, key, buff, sizeof(buff)) > 0)

	// ---- WiFi opmode ----

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

	if (GET_ARG("ap_enable")) {
		dbg("Enable AP: %s", buff);
		int enable = atoi(buff);

		if (enable) {
			wificonf->opmode |= SOFTAP_MODE;
		} else {
			wificonf->opmode &= ~SOFTAP_MODE;
		}
	}

	if (GET_ARG("sta_enable")) {
		dbg("Enable STA: %s", buff);
		int enable = atoi(buff);

		if (enable) {
			wificonf->opmode |= STATION_MODE;
		} else {
			wificonf->opmode &= ~STATION_MODE;
		}
	}

	// ---- AP transmit power ----

	if (GET_ARG("tpw")) {
		dbg("Setting AP power to: %s", buff);
		int tpw = atoi(buff);
		if (tpw >= 0 && tpw <= 82) { // 0 actually isn't 0 but quite low. 82 is very strong
			if (wificonf->tpw != tpw) {
				wificonf->tpw = (u8) tpw;
				wifi_change_flags.ap = true;
			}
		} else {
			warn("tpw %s out of allowed range 0-82.", buff);
			redir_url += sprintf(redir_url, "tpw,");
		}
	}

	// ---- AP channel (applies in AP-only mode) ----

	if (GET_ARG("ap_channel")) {
		info("ap_channel = %s", buff);
		int channel = atoi(buff);
		if (channel > 0 && channel < 15) {
			if (wificonf->ap_channel != channel) {
				wificonf->ap_channel = (u8) channel;
				wifi_change_flags.ap = true;
			}
		} else {
			warn("Bad channel value \"%s\", allowed 1-14", buff);
			redir_url += sprintf(redir_url, "ap_channel,");
		}
	}

	// ---- SSID name in AP mode ----

	if (GET_ARG("ap_ssid")) {
		// Replace all invalid ASCII with underscores
		int i;
		for (i = 0; i < 32; i++) {
			char c = buff[i];
			if (c == 0) break;
			if (c < 32 || c >= 127) buff[i] = '_';
		}
		buff[i] = 0;

		if (strlen(buff) > 0) {
			if (!streq(wificonf->ap_ssid, buff)) {
				info("Setting SSID to \"%s\"", buff);
				strncpy_safe(wificonf->ap_ssid, buff, SSID_LEN);
				wifi_change_flags.ap = true;
			}
		} else {
			warn("Bad SSID len.");
			redir_url += sprintf(redir_url, "ap_ssid,");
		}
	}

	// ---- AP password ----

	if (GET_ARG("ap_password")) {
		// Users are free to use any stupid shit in ther password,
		// but it may lock them out.
		if (strlen(buff) == 0 || (strlen(buff) >= 8 && strlen(buff) < PASSWORD_LEN-1)) {
			if (!streq(wificonf->ap_password, buff)) {
				info("Setting AP password to \"%s\"", buff);
				strncpy_safe(wificonf->ap_password, buff, PASSWORD_LEN);
				wifi_change_flags.ap = true;
			}
		} else {
			warn("Bad password len.");
			redir_url += sprintf(redir_url, "ap_password,");
		}
	}

	// ---- Hide AP network (do not announce) ----

	if (GET_ARG("ap_hidden")) {
		dbg("AP hidden = %s", buff);
		int hidden = atoi(buff);
		if (hidden != wificonf->ap_hidden) {
			wificonf->ap_hidden = (hidden != 0);
			wifi_change_flags.ap = true;
		}
	}

	// ---- AP DHCP server lease time ----

	if (GET_ARG("ap_dhcp_time")) {
		dbg("Setting DHCP lease time to: %s min.", buff);
		int min = atoi(buff);
		if (min >= 1 && min <= 2880) {
			if (wificonf->ap_dhcp_time != min) {
				wificonf->ap_dhcp_time = (u16) min;
				wifi_change_flags.ap = true;
			}
		} else {
			warn("Lease time %s out of allowed range 1-2880.", buff);
			redir_url += sprintf(redir_url, "ap_dhcp_time,");
		}
	}

	// ---- AP DHCP start and end IP ----

	if (GET_ARG("ap_dhcp_start")) {
		dbg("Setting DHCP range start IP to: \"%s\"", buff);
		u32 ip = ipaddr_addr(buff);
		if (ip != 0) {
			if (wificonf->ap_dhcp_range.start_ip.addr != ip) {
				wificonf->ap_dhcp_range.start_ip.addr = ip;
				wifi_change_flags.ap = true;
			}
		} else {
			warn("Bad IP: %s", buff);
			redir_url += sprintf(redir_url, "ap_dhcp_start,");
		}
	}

	if (GET_ARG("ap_dhcp_end")) {
		dbg("Setting DHCP range end IP to: \"%s\"", buff);
		u32 ip = ipaddr_addr(buff);
		if (ip != 0) {
			if (wificonf->ap_dhcp_range.end_ip.addr != ip) {
				wificonf->ap_dhcp_range.end_ip.addr = ip;
				wifi_change_flags.ap = true;
			}
		} else {
			warn("Bad IP: %s", buff);
			redir_url += sprintf(redir_url, "ap_dhcp_end,");
		}
	}

	// ---- AP local address & config ----

	if (GET_ARG("ap_addr_ip")) {
		dbg("Setting AP local IP to: \"%s\"", buff);
		u32 ip = ipaddr_addr(buff);
		if (ip != 0) {
			if (wificonf->ap_addr.ip.addr != ip) {
				wificonf->ap_addr.ip.addr = ip;
				wificonf->ap_addr.gw.addr = ip; // always the same, we're the router here
				wifi_change_flags.ap = true;
			}
		} else {
			warn("Bad IP: %s", buff);
			redir_url += sprintf(redir_url, "ap_addr_ip,");
		}
	}

	if (GET_ARG("ap_addr_mask")) {
		dbg("Setting AP local IP netmask to: \"%s\"", buff);
		u32 ip = ipaddr_addr(buff);
		if (ip != 0) {
			if (wificonf->ap_addr.netmask.addr != ip) {
				// ideally this should be checked to match the IP.
				// Let's hope users know what they're doing
				wificonf->ap_addr.netmask.addr = ip;
				wifi_change_flags.ap = true;
			}
		} else {
			warn("Bad IP mask: %s", buff);
			redir_url += sprintf(redir_url, "ap_addr_mask,");
		}
	}

	// ---- Station SSID (to connect to) ----

	if (GET_ARG("sta_ssid")) {
		if (!streq(wificonf->sta_ssid, buff)) {
			// No verification needed, at worst it fails to connect
			info("Setting station SSID to: \"%s\"", buff);
			strncpy_safe(wificonf->sta_ssid, buff, SSID_LEN);
			wifi_change_flags.sta = true;
		}
	}

	// ---- Station password (empty for none is allowed) ----

	if (GET_ARG("sta_password")) {
		if (!streq(wificonf->sta_password, buff)) {
			// No verification needed, at worst it fails to connect
			info("Setting station password to: \"%s\"", buff);
			strncpy_safe(wificonf->sta_password, buff, PASSWORD_LEN);
			wifi_change_flags.sta = true;
		}
	}

	// ---- Station enable/disable DHCP ----

	// DHCP enable / disable (disable means static IP is enabled)
	if (GET_ARG("sta_dhcp_enable")) {
		dbg("DHCP enable = %s", buff);
		int enable = atoi(buff);
		if (wificonf->sta_dhcp_enable != enable) {
			wificonf->sta_dhcp_enable = (bool)enable;
			wifi_change_flags.sta = true;
		}
	}

	// ---- Station IP config (Static IP) ----

	if (GET_ARG("sta_addr_ip")) {
		dbg("Setting Station mode static IP to: \"%s\"", buff);
		u32 ip = ipaddr_addr(buff);
		if (ip != 0) {
			if (wificonf->sta_addr.ip.addr != ip) {
				wificonf->sta_addr.ip.addr = ip;
				wifi_change_flags.sta = true;
			}
		} else {
			warn("Bad IP: %s", buff);
			redir_url += sprintf(redir_url, "sta_addr_ip,");
		}
	}

	if (GET_ARG("sta_addr_mask")) {
		dbg("Setting Station mode static IP netmask to: \"%s\"", buff);
		u32 ip = ipaddr_addr(buff);
		if (ip != 0 && ip != 0xFFFFFFFFUL) {
			if (wificonf->sta_addr.netmask.addr != ip) {
				wificonf->sta_addr.netmask.addr = ip;
				wifi_change_flags.sta = true;
			}
		} else {
			warn("Bad IP mask: %s", buff);
			redir_url += sprintf(redir_url, "sta_addr_mask,");
		}
	}

	if (GET_ARG("sta_addr_gw")) {
		dbg("Setting Station mode static IP default gateway to: \"%s\"", buff);
		u32 ip = ipaddr_addr(buff);
		if (ip != 0) {
			if (wificonf->sta_addr.gw.addr != ip) {
				wificonf->sta_addr.gw.addr = ip;
				wifi_change_flags.sta = true;
			}
		} else {
			warn("Bad gw IP: %s", buff);
			redir_url += sprintf(redir_url, "sta_addr_gw,");
		}
	}

	if (redir_url_buf[10] == 0) {
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
httpd_cgi_state ICACHE_FLASH_ATTR tplWlan(HttpdConnData *connData, char *token, void **arg)
{
	char buff[100];
	int x;
	int connectStatus;

	if (token == NULL) {
		// We're done
		return HTTPD_CGI_DONE;
	}

	strcpy(buff, ""); // fallback

	if (streq(token, "opmode_name")) {
		strcpy(buff, opmode2str(wificonf->opmode));
	}
	else if (streq(token, "opmode")) {
		sprintf(buff, "%d", wificonf->opmode);
	}
	else if (streq(token, "sta_enable")) {
		sprintf(buff, "%d", (wificonf->opmode & STATION_MODE) != 0);
	}
	else if (streq(token, "ap_enable")) {
		sprintf(buff, "%d", (wificonf->opmode & SOFTAP_MODE) != 0);
	}
	else if (streq(token, "tpw")) {
		sprintf(buff, "%d", wificonf->tpw);
	}
	else if (streq(token, "ap_channel")) {
		sprintf(buff, "%d", wificonf->ap_channel);
	}
	else if (streq(token, "ap_ssid")) {
		sprintf(buff, "%s", wificonf->ap_ssid);
	}
	else if (streq(token, "ap_password")) {
		sprintf(buff, "%s", wificonf->ap_password);
	}
	else if (streq(token, "ap_hidden")) {
		sprintf(buff, "%d", wificonf->ap_hidden);
	}
	else if (streq(token, "ap_dhcp_time")) {
		sprintf(buff, "%d", wificonf->ap_dhcp_time);
	}
	else if (streq(token, "ap_dhcp_start")) {
		sprintf(buff, IPSTR, GOOD_IP2STR(wificonf->ap_dhcp_range.start_ip.addr));
	}
	else if (streq(token, "ap_dhcp_end")) {
		sprintf(buff, IPSTR, GOOD_IP2STR(wificonf->ap_dhcp_range.end_ip.addr));
	}
	else if (streq(token, "ap_addr_ip")) {
		sprintf(buff, IPSTR, GOOD_IP2STR(wificonf->ap_addr.ip.addr));
	}
	else if (streq(token, "ap_addr_mask")) {
		sprintf(buff, IPSTR, GOOD_IP2STR(wificonf->ap_addr.netmask.addr));
	}
	else if (streq(token, "sta_ssid")) {
		sprintf(buff, "%s", wificonf->sta_ssid);
	}
	else if (streq(token, "sta_password")) {
		sprintf(buff, "%s", wificonf->sta_password);
	}
	else if (streq(token, "sta_dhcp_enable")) {
		sprintf(buff, "%d", wificonf->sta_dhcp_enable);
	}
	else if (streq(token, "sta_addr_ip")) {
		sprintf(buff, IPSTR, GOOD_IP2STR(wificonf->sta_addr.ip.addr));
	}
	else if (streq(token, "ap_addr_mask")) {
		sprintf(buff, IPSTR, GOOD_IP2STR(wificonf->sta_addr.netmask.addr));
	}
	else if (streq(token, "ap_addr_gw")) {
		sprintf(buff, IPSTR, GOOD_IP2STR(wificonf->sta_addr.gw.addr));
	}
	else if (streq(token, "sta_rssi")) {
		sprintf(buff, "%d", wifi_station_get_rssi());
	}
	else if (streq(token, "sta_active_ssid")) {
		// For display of our current SSID
		connectStatus = wifi_station_get_connect_status();
		x = wifi_get_opmode();
		if (x == SOFTAP_MODE || connectStatus != STATION_GOT_IP) {
			strcpy(buff, "");
		}
		else {
			struct station_config staconf;
			wifi_station_get_config(&staconf);
			strcpy(buff, (char *) staconf.ssid);
		}
	}
	else if (streq(token, "sta_active_ip")) {
		x = wifi_get_opmode();
		connectStatus = wifi_station_get_connect_status();

		if (x == SOFTAP_MODE || connectStatus != STATION_GOT_IP) {
			strcpy(buff, "");
		}
		else {
			struct ip_info info;
			wifi_get_ip_info(STATION_IF, &info);
			sprintf(buff, "ip: "IPSTR", mask: "IPSTR", gw: "IPSTR,
					GOOD_IP2STR(info.ip.addr),
					GOOD_IP2STR(info.netmask.addr),
					GOOD_IP2STR(info.gw.addr));
		}
	}

	httpdSend(connData, buff, -1);
	return HTTPD_CGI_DONE;
}
