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

/** reset_later() timer */
static ETSTimer resetTmr;

/**
 * Callback for reset_later()
 */
static void ICACHE_FLASH_ATTR resetTmrCb(void *arg)
{
	system_restart();
}

/**
 * Schedule a reset
 * @param ms reset delay (milliseconds)
 */
static void ICACHE_FLASH_ATTR reset_later(int ms)
{
	os_timer_disarm(&resetTmr);
	os_timer_setfn(&resetTmr, resetTmrCb, NULL);
	os_timer_arm(&resetTmr, ms, false);
}

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

/** Temp store for new ap info. */
static struct station_config stconf;

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
 * Actually connect to a station. This routine is timed because I had problems
 * with immediate connections earlier. It probably was something else that caused it,
 * but I can't be arsed to put the code back :P
 */
static void ICACHE_FLASH_ATTR cgiWiFiConnect_do(void *arg)
{
	int x;
	dbg("Try to connect to AP...");

	wifi_station_disconnect();
	wifi_station_set_config(&stconf);
	wifi_station_connect();

	x = wifi_get_opmode();
	connTryStatus = CONNTRY_WORKING;
	if (x != STATION_MODE) {
		//Schedule check
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
	char essid[128];
	char passwd[128];
	static os_timer_t reassTimer;

	if (connData->conn == NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	int ssilen = httpdFindArg(connData->post->buff, "essid", essid, sizeof(essid));
	int passlen = httpdFindArg(connData->post->buff, "passwd", passwd, sizeof(passwd));

	if (ssilen == -1 || passlen == -1) {
		error("Not rx needed args!");
		httpdRedirect(connData, "/wifi");
	}
	else {
		strncpy((char *) stconf.ssid, essid, 32);
		strncpy((char *) stconf.password, passwd, 64);
		info("Try to connect to AP %s pw %s", essid, passwd);

		//Schedule disconnect/connect
		os_timer_disarm(&reassTimer);
		os_timer_setfn(&reassTimer, cgiWiFiConnect_do, NULL);
		// redirect & start connecting a little bit later
		os_timer_arm(&reassTimer, 2000, 0); // was 500, increased so the connecting page has time to load

		connTryStatus = CONNTRY_IDLE;
		httpdRedirect(connData, "/wifi/connecting");
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

/**
 * Universal CGI endpoint to set WiFi params.
 * Note that some may cause a (delayed) restart.
 *
 * Args:
 * - ap_ch = channel 1-14
 * - ap_ssid = SSID name for AP mode
 * - opmode = WiFi mode (resets device)
 * - hostname = set client hostname
 * - tpw = set transmit power
 * - sta_dhcp_lt = DHCP server lease time
 * - sta_ip   = station mode static IP
 * - sta_mask = station mode static IP mask (apply only if 'ip' is also sent)
 * - sta_gw   = station mode default gateway (apply only if 'ip' is also sent)
 *              (can be left out, then 0.0.0.0 is used and outbound connections won't work -
 *              but we're normally not making any)
 * - dhcp = enable or disable DHCP on the station interface
 */
httpd_cgi_state ICACHE_FLASH_ATTR cgiWiFiSetParams(HttpdConnData *connData)
{
	int len, len2, len3;
	char buff[50];
	char buff2[50];
	char buff3[50];

	// TODO change so that settings are not applied immediately, but persisted first
	// TODO apply temporary changes like static IP in wifi event CBs

	if (connData->conn == NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	// AP channel (applies in AP-only mode)
	len = httpdFindArg(connData->getArgs, "ap_ch", buff, sizeof(buff));
	if (len > 0) {
		info("Setting WiFi channel for AP-only mode to: %s", buff);
		int channel = atoi(buff);
		if (channel > 0 && channel < 15) {
			dbg("Setting channel=%d", channel);

			struct softap_config wificfg;
			wifi_softap_get_config(&wificfg);
			wificfg.channel = (uint8) channel;
			wifi_softap_set_config(&wificfg);
		} else {
			warn("Bad channel value %s, allowed 1-14", buff);
		}
	}

	// SSID name in AP mode
	len = httpdFindArg(connData->getArgs, "ap_ssid", buff, sizeof(buff));
	if (len > 0) {
		int i;
		for (i = 0; i < 32; i++) {
			char c = buff[i];
			if (c == 0) break;
			if (c < 32 || c >= 127) buff[i] = '_';
		}
		buff[i] = 0;

		info("Setting SSID to %s", buff);

		struct softap_config wificfg;
		wifi_softap_get_config(&wificfg);
		sprintf((char *) wificfg.ssid, buff);
		wificfg.ssid_len = strlen((char *) wificfg.ssid);
		wifi_softap_set_config(&wificfg);
	}

	// WiFi mode
	len = httpdFindArg(connData->getArgs, "opmode", buff, sizeof(buff));
	if (len > 0) {
		dbg("Setting WiFi opmode to: %s", buff);
		int mode = atoi(buff);
		if (mode > NULL_MODE && mode < MAX_MODE) {
			wifi_set_opmode(mode);
			reset_later(200);
		} else {
			warn("Bad opmode value %s", buff);
		}
	}

	// Hostname in station mode (for DHCP)
	len = httpdFindArg(connData->getArgs, "hostname", buff, sizeof(buff));
	if (len > 0) {
		dbg("Setting station sta_hostname to: %s", buff);
		wifi_station_set_hostname(buff);
		// TODO persistency, re-apply on boot
	}

	// Hostname in station mode (for DHCP)
	len = httpdFindArg(connData->getArgs, "tpw", buff, sizeof(buff));
	if (len > 0) {
		dbg("Setting AP power to: %s", buff);
		int tpw = atoi(buff);
		// min tpw to avoid user locking themselves out TODO verify
		if (tpw >= 0 && tpw <= 82) {
			// TODO persistency, re-apply on boot
			system_phy_set_max_tpw(tpw);
		} else {
			warn("tpw %s out of allowed range 0-82.", buff);
		}
	}

	// DHCP server lease time
	len = httpdFindArg(connData->getArgs, "ap_dhcp_lt", buff, sizeof(buff));
	if (len > 0) {
		dbg("Setting DHCP lease time to: %s min.", buff);
		int min = atoi(buff);
		if (min >= 1 && min <= 2880) {
			// TODO persistency, re-apply on boot
			// TODO set only if we're in the right opmode
			wifi_softap_set_dhcps_lease_time(min);
		} else {
			warn("Lease time %s out of allowed range 1-2880.", buff);
		}
	}

	// DHCP enable / disable (disable means static IP is enabled)
	len = httpdFindArg(connData->getArgs, "sta_dhcp", buff, sizeof(buff));
	if (len > 0) {
		dbg("DHCP enable =  %s", buff);
		int enable = atoi(buff);
		if (enable != 0) {
			wifi_station_dhcpc_stop();
		} else {
			wifi_station_dhcpc_start();
		}
		// TODO persistency
	}

	// Static IP
	len = httpdFindArg(connData->getArgs, "sta_ip", buff, sizeof(buff));
	len2 = httpdFindArg(connData->getArgs, "sta_mask", buff2, sizeof(buff2));
	len3 = httpdFindArg(connData->getArgs, "sta_gw", buff3, sizeof(buff3));
	if (len > 0) {
		// TODO set only if we're in the right opmode
		// TODO persistency
		dbg("Setting static IP = %s", buff);
		struct ip_info ipinfo;
		ipinfo.ip.addr = ipaddr_addr(buff);
		ipinfo.netmask.addr = IPADDR_NONE;
		ipinfo.gw.addr = IPADDR_NONE;
		if (len2 > 0) {
			dbg("Netmask = %s", buff2);
			ipinfo.netmask.addr = ipaddr_addr(buff2);
		}
		if (len3 > 0) {
			dbg("Gateway = %s", buff3);
			ipinfo.gw.addr = ipaddr_addr(buff3);
		}
		// TODO ...
		wifi_station_dhcpc_stop();
		wifi_set_ip_info(STATION_IF, &ipinfo);
	}

	httpdRedirect(connData, "/wifi");
	return HTTPD_CGI_DONE;
}


//Template code for the WLAN page.
httpd_cgi_state ICACHE_FLASH_ATTR tplWlan(HttpdConnData *connData, char *token, void **arg)
{
	char buff[500];
	int x;
	int connectStatus;
	static struct station_config stconf;
	static struct softap_config apconf;

	if (token == NULL) {
		// We're done
		return HTTPD_CGI_DONE;
	}

	wifi_station_get_config(&stconf);
	wifi_softap_get_config(&apconf);


	strcpy(buff, "Unknown");
	if (streq(token, "WiFiMode")) {
		x = wifi_get_opmode();
		strcpy(buff, opmode2str(x));
	}
	else if (streq(token, "WiFiModeNum")) {
		x = wifi_get_opmode();
		sprintf(buff, "%d", x);
	}
	else if (streq(token, "WiFiChannel")) {
		sprintf(buff, "%d", apconf.channel);
	}
	else if (streq(token, "APName")) {
		sprintf(buff, "%s", apconf.ssid);
	}
	else if (streq(token, "StaIP")) {
		x = wifi_get_opmode();
		connectStatus = wifi_station_get_connect_status();

		if (x == SOFTAP_MODE || connectStatus != STATION_GOT_IP) {
			strcpy(buff, "");
		}
		else {
			struct ip_info info;
			wifi_get_ip_info(STATION_IF, &info);
			sprintf(buff, IPSTR, GOOD_IP2STR(info.ip.addr));
		}
	}
	else if (streq(token, "StaSSID")) {
		connectStatus = wifi_station_get_connect_status();
		x = wifi_get_opmode();
		if (x == SOFTAP_MODE || connectStatus != STATION_GOT_IP) {
			strcpy(buff, "");
		}
		else {
			strcpy(buff, (char *) stconf.ssid);
		}
	}
	else if (streq(token, "WiFiPasswd")) {
		strcpy(buff, (char *) stconf.password);
	}
	else if (streq(token, "WiFiapwarn")) {
		// TODO get rid of this
		x = wifi_get_opmode();
		if (x == SOFTAP_MODE) { // 2
			strcpy(buff, "<a href=\"/wifi/setmode?mode=3\">Enable client</a> for scanning.");
		}
		else if (x == STATIONAP_MODE) { // 3
			strcpy(buff,
				   "Switch: <a href=\"/wifi/setmode?mode=1\">Client only</a>, <a href=\"/wifi/setmode?mode=2\">AP only</a>");
		}
		else { // 1
			strcpy(buff,
				   "Switch: <a href=\"/wifi/setmode?mode=3\">Client+AP</a>, <a href=\"/wifi/setmode?mode=2\">AP only</a>");
		}
	}
	httpdSend(connData, buff, -1);
	return HTTPD_CGI_DONE;
}
