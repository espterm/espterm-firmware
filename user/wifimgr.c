//
// Created by MightyPork on 2017/07/08.
//

#include "wifimgr.h"
#include "persist.h"
#include "cgi_logging.h"
#include "config_xmacros.h"

WiFiConfigBundle * const wificonf = &persist.current.wificonf;
WiFiConfChangeFlags wifi_change_flags;

enum xset_result ICACHE_FLASH_ATTR
xset_wifi_lease_time(const char *name, u16 *field, const char *buff, const void *arg)
{
	cgi_dbg("Setting %s = %s min", name, buff);
	int min = atoi(buff);
	if (min >= 1 && min <= 2880) {
		if (*field != min) {
			*field = (u16) min;
			return XSET_SET;
		}
		return XSET_UNCHANGED;
	} else {
		cgi_warn("Lease time %s out of allowed range 1-2880.", buff);
		return XSET_FAIL;
	}
}

enum xset_result ICACHE_FLASH_ATTR
xset_wifi_opmode(const char *name, u8 *field, const char *buff, const void *arg)
{
	cgi_dbg("Setting %s = %s", name, buff);
	int mode = atoi(buff);
	if (mode > NULL_MODE && mode < MAX_MODE) {
		if (*field != mode) {
			*field = (WIFI_MODE) mode;
			return XSET_SET;
		}
		return XSET_UNCHANGED; // opmode does not use flags
	} else {
		cgi_warn("Bad opmode value \"%s\"", buff);
		return XSET_FAIL;
	}
}

enum xset_result ICACHE_FLASH_ATTR
xset_wifi_tpw(const char *name, u8 *field, const char *buff, const void *arg)
{
	cgi_dbg("Setting %s = %s", name, buff);
	int tpw = atoi(buff);
	if (tpw >= 0 && tpw <= 82) { // 0 actually isn't 0 but quite low. 82 is very strong
		if (*field != tpw) {
			*field = (u8) tpw;
			return XSET_SET;
		}
		return XSET_UNCHANGED;
	} else {
		cgi_warn("tpw %s out of allowed range 0-82.", buff);
		return XSET_FAIL;
	}
}

enum xset_result ICACHE_FLASH_ATTR
xset_wifi_ap_channel(const char *name, u8 *field, const char *buff, const void *arg)
{
	cgi_dbg("Setting %s = %s", name, buff);
	int channel = atoi(buff);
	if (channel > 0 && channel < 15) {
		if (*field != channel) {
			*field = (u8) channel;
			return XSET_SET;
		}
		return XSET_UNCHANGED;
	} else {
		cgi_warn("Bad channel value \"%s\", allowed 1-14", buff);
		return XSET_FAIL;
	}
}

enum xset_result ICACHE_FLASH_ATTR
xset_wifi_ssid(const char *name, uchar *field, const char *buff, const void *arg)
{
	u8 buff2[SSID_LEN];

	bool want_subs = arg!=0;

	int i;
	for (i = 0; i < SSID_LEN; i++) {
		char c = buff[i];
		if (c == 0) break;
		if (want_subs && (c < 32 || c >= 127)) c = '_';
		buff2[i] = (u8) c;
	}
	buff2[i] = 0;

	cgi_dbg("Setting %s = %s", name, buff);
	if (strlen((char *)buff2) > 0) {
		if (!streq(field, buff2)) {
			strncpy_safe(field, buff2, SSID_LEN);
			return XSET_SET;
		}
		return XSET_UNCHANGED;
	} else {
		cgi_warn("Bad SSID len.");
		return XSET_FAIL;
	}
}

/** Set PW - allow len 0 or 8-64 */
enum xset_result ICACHE_FLASH_ATTR
xset_wifi_pwd(const char *name, uchar *field, const char *buff, const void *arg)
{
	cgi_dbg("Setting %s = %s", name, buff);
	if (strlen(buff) == 0 || (strlen(buff) >= 8 && strlen(buff) < PASSWORD_LEN-1)) {
		if (!streq(field, buff)) {
			strncpy_safe(field, buff, PASSWORD_LEN);
			return XSET_SET;
		}
		return XSET_UNCHANGED;
	} else {
		cgi_warn("Bad password len.");
		return XSET_FAIL;
	}
}


int ICACHE_FLASH_ATTR getStaIpAsString(char *buffer)
{
	WIFI_MODE x = wifi_get_opmode();
	STATION_STATUS connectStatus = wifi_station_get_connect_status();

	if (x == SOFTAP_MODE || connectStatus != STATION_GOT_IP || wificonf->opmode == SOFTAP_MODE) {
		strcpy(buffer, "");
		return 0;
	}
	else {
		struct ip_info info;
		wifi_get_ip_info(STATION_IF, &info);
		return sprintf(buffer, IPSTR, GOOD_IP2STR(info.ip.addr));
	}
}

/**
 * Restore defaults in the WiFi config block.
 * This is to be called if the WiFi config is corrupted on startup,
 * before applying the config.
 */
void ICACHE_FLASH_ATTR
wifimgr_restore_defaults(void)
{
	u8 mac[6];
	wifi_get_macaddr(SOFTAP_IF, mac);

	wificonf->opmode = STATIONAP_MODE; // Client+AP, so we can scan without having to enable Station
	wificonf->tpw = 20;
	wificonf->ap_channel = 1;
	sprintf((char *) wificonf->ap_ssid, "TERM-%02X%02X%02X", mac[3], mac[4], mac[5]);
	wificonf->ap_password[0] = 0; // PSK2 always if password is not null.
	wificonf->ap_hidden = false;

	IP4_ADDR(&wificonf->ap_addr_ip, 192, 168, 4, 1);
	IP4_ADDR(&wificonf->ap_addr_mask, 255, 255, 255, 0);

	IP4_ADDR(&wificonf->ap_dhcp_start, 192, 168, 4, 100);
	IP4_ADDR(&wificonf->ap_dhcp_end, 192, 168, 4, 200);
	wificonf->ap_dhcp_time = 120;

	// --- Client config ---
	wificonf->sta_ssid[0] = 0;
	wificonf->sta_password[0] = 0;
	wificonf->sta_dhcp_enable = true;

	IP4_ADDR(&wificonf->sta_addr_ip, 192, 168, 0, (mac[5] == 1 ? 2 : mac[5])); // avoid being the same as "default gw"
	IP4_ADDR(&wificonf->sta_addr_mask, 255, 255, 255, 0);
	IP4_ADDR(&wificonf->sta_addr_gw, 192, 168, 0, 1); // a common default...
}

static void ICACHE_FLASH_ATTR
configure_station(void)
{
	wifi_info("[WiFi] Configuring Station mode...");
	struct station_config conf;
	strcpy((char *) conf.ssid, (char *) wificonf->sta_ssid);
	strcpy((char *) conf.password, (char *) wificonf->sta_password);
	wifi_dbg("[WiFi] Connecting to \"%s\"%s password", conf.ssid, conf.password[0]!=0?" using saved":", no");
	conf.bssid_set = 0;
	conf.bssid[0] = 0;
	wifi_station_disconnect();
	wifi_station_set_config_current(&conf);

	if (wificonf->sta_dhcp_enable) {
		wifi_dbg("[WiFi] Starting DHCP...");
		if (!wifi_station_dhcpc_start()) {
			error("[WiFi] DHCP failed to start!");
			return;
		}
	}
	else {
		wifi_info("[WiFi] Setting up static IP...");
		wifi_dbg("[WiFi] Client.ip   = "IPSTR, GOOD_IP2STR(wificonf->sta_addr_ip.addr));
		wifi_dbg("[WiFi] Client.mask = "IPSTR, GOOD_IP2STR(wificonf->sta_addr_mask.addr));
		wifi_dbg("[WiFi] Client.gw   = "IPSTR, GOOD_IP2STR(wificonf->sta_addr_gw.addr));

		wifi_station_dhcpc_stop();
		// Load static IP config
		struct ip_info ipstruct;
		ipstruct.ip.addr = wificonf->sta_addr_ip.addr;
		ipstruct.netmask.addr = wificonf->sta_addr_mask.addr;
		ipstruct.gw.addr = wificonf->sta_addr_gw.addr;

		if (!wifi_set_ip_info(STATION_IF, &ipstruct)) {
			error("[WiFi] Error setting static IP!");
			return;
		}
	}

	wifi_info("[WiFi] Trying to connect to AP...");
	wifi_station_connect();
}

static void ICACHE_FLASH_ATTR
configure_ap(void)
{
	bool suc;

	wifi_info("[WiFi] Configuring SoftAP mode...");
	// AP is enabled
	struct softap_config conf;
	conf.channel = wificonf->ap_channel;
	strcpy((char *) conf.ssid, (char *) wificonf->ap_ssid);
	strcpy((char *) conf.password, (char *) wificonf->ap_password);
	conf.authmode = (wificonf->ap_password[0] == 0 ? AUTH_OPEN : AUTH_WPA2_PSK);
	conf.ssid_len = (uint8_t) strlen((char *) conf.ssid);
	conf.ssid_hidden = (uint8) wificonf->ap_hidden;
	conf.max_connection = 4; // default 4 (max possible)
	conf.beacon_interval = 100; // default 100 ms

	// Set config
	//ETS_UART_INTR_DISABLE();
	suc = wifi_softap_set_config_current(&conf);
	//ETS_UART_INTR_ENABLE();
	if (!suc) {
		error("[WiFi] AP config set fail!");
		return;
	}

	// Set IP
	wifi_info("[WiFi] Configuring SoftAP local IP...");
	wifi_dbg("[WiFi] SoftAP.ip   = "IPSTR, GOOD_IP2STR(wificonf->ap_addr_ip.addr));
	wifi_dbg("[WiFi] SoftAP.mask = "IPSTR, GOOD_IP2STR(wificonf->ap_addr_mask.addr));

	wifi_softap_dhcps_stop();

	// Configure DHCP
	struct ip_info ipstruct;
	ipstruct.ip.addr = wificonf->ap_addr_ip.addr;
	ipstruct.netmask.addr = wificonf->ap_addr_mask.addr;
	ipstruct.gw.addr = wificonf->ap_addr_ip.addr;

	if (!wifi_set_ip_info(SOFTAP_IF, &ipstruct)) {
		error("[WiFi] IP set fail!");
		return;
	}

	wifi_info("[WiFi] Configuring SoftAP DHCP server...");
	wifi_dbg("[WiFi] DHCP.start = "IPSTR, GOOD_IP2STR(wificonf->ap_dhcp_start.addr));
	wifi_dbg("[WiFi] DHCP.end   = "IPSTR, GOOD_IP2STR(wificonf->ap_dhcp_end.addr));
	wifi_dbg("[WiFi] DHCP.lease = %d minutes", wificonf->ap_dhcp_time);

	struct dhcps_lease dhcpstruct;
	dhcpstruct.start_ip = wificonf->ap_dhcp_start;
	dhcpstruct.end_ip = wificonf->ap_dhcp_end;
	dhcpstruct.enable = 1; // ???
	if (!wifi_softap_set_dhcps_lease(&dhcpstruct)) {
		error("[WiFi] DHCP address range set fail!");
		return;
	}

	if (!wifi_softap_set_dhcps_lease_time(wificonf->ap_dhcp_time)) {
		error("[WiFi] DHCP lease time set fail!");
		return;
	}

	// some weird magic shit about router
	uint8 mode = 1;
	wifi_softap_set_dhcps_offer_option(OFFER_ROUTER, &mode);

	if (!wifi_softap_dhcps_start()) {
		error("[WiFi] Failed to start DHCP server!");
		return;
	}
}

/**
 * Register the WiFi event listener, cycle WiFi, apply settings
 */
void ICACHE_FLASH_ATTR
wifimgr_apply_settings(void)
{
	wifi_info("[WiFi] Initializing...");

//	char buff[64];
//#define XSTRUCT wificonf
//#define X XDUMP_FIELD
//	XTABLE_WIFICONF
//#undef X

	// !!! Update to current version !!!

	// Force wifi cycle
	// Disconnect - may not be needed?
	WIFI_MODE opmode = wifi_get_opmode();

	bool is_sta = wificonf->opmode & STATION_MODE;
	bool is_ap = wificonf->opmode & SOFTAP_MODE;

	if ((wificonf->opmode & STATION_MODE) && !(opmode & STATION_MODE)) {
		wifi_change_flags.sta = true;
	}

	if ((wificonf->opmode & SOFTAP_MODE) && !(opmode & SOFTAP_MODE)) {
		wifi_change_flags.ap = true;
	}

	if (opmode != wificonf->opmode) {
		wifi_set_opmode_current((WIFI_MODE) wificonf->opmode);
	}

	// Configure the client
	if (is_sta && wifi_change_flags.sta) {
		configure_station();
	}

	// Configure the AP
	if (is_ap && wifi_change_flags.ap) {
		configure_ap();
	}

	// tpw seems to be common - but info is scarce
	// at any rate seems to do no harm to have it here
	system_phy_set_max_tpw(wificonf->tpw);

	wifi_change_flags.ap = false;
	wifi_change_flags.sta = false;

	wifi_info("[WiFi] WiFi settings applied.");
}
