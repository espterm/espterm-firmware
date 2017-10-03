//
// Created by MightyPork on 2017/07/08.
//

#include "wifimgr.h"
#include "persist.h"

WiFiConfigBundle * const wificonf = &persist.current.wificonf;
WiFiConfChangeFlags wifi_change_flags;

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

	IP4_ADDR(&wificonf->ap_addr.ip, 192, 168, 4, 1);
	IP4_ADDR(&wificonf->ap_addr.netmask, 255, 255, 255, 0);
	wificonf->ap_addr.gw.addr = wificonf->ap_addr.gw.addr;

	IP4_ADDR(&wificonf->ap_dhcp_range.start_ip, 192, 168, 4, 100);
	IP4_ADDR(&wificonf->ap_dhcp_range.end_ip, 192, 168, 4, 200);
	wificonf->ap_dhcp_range.enable = 1; // this will never get changed, idk why it's even there
	wificonf->ap_dhcp_time = 120;

	// --- Client config ---
	wificonf->sta_ssid[0] = 0;
	wificonf->sta_password[0] = 0;
	wificonf->sta_dhcp_enable = true;

	IP4_ADDR(&wificonf->sta_addr.ip, 192, 168, 0, (mac[5] == 1 ? 2 : mac[5])); // avoid being the same as "default gw"
	IP4_ADDR(&wificonf->sta_addr.netmask, 255, 255, 255, 0);
	IP4_ADDR(&wificonf->sta_addr.gw, 192, 168, 0, 1);
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
		wifi_dbg("[WiFi] Client.ip   = "IPSTR, GOOD_IP2STR(wificonf->sta_addr.ip.addr));
		wifi_dbg("[WiFi] Client.mask = "IPSTR, GOOD_IP2STR(wificonf->sta_addr.netmask.addr));
		wifi_dbg("[WiFi] Client.gw   = "IPSTR, GOOD_IP2STR(wificonf->sta_addr.gw.addr));

		wifi_station_dhcpc_stop();
		// Load static IP config
		if (!wifi_set_ip_info(STATION_IF, &wificonf->sta_addr)) {
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
	conf.ssid_hidden = wificonf->ap_hidden;
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
	wifi_dbg("[WiFi] SoftAP.ip   = "IPSTR, GOOD_IP2STR(wificonf->ap_addr.ip.addr));
	wifi_dbg("[WiFi] SoftAP.mask = "IPSTR, GOOD_IP2STR(wificonf->ap_addr.netmask.addr));
	wifi_dbg("[WiFi] SoftAP.gw   = "IPSTR, GOOD_IP2STR(wificonf->ap_addr.gw.addr));

	wifi_softap_dhcps_stop();

	// Configure DHCP
	if (!wifi_set_ip_info(SOFTAP_IF, &wificonf->ap_addr)) {
		error("[WiFi] IP set fail!");
		return;
	}

	wifi_info("[WiFi] Configuring SoftAP DHCP server...");
	wifi_dbg("[WiFi] DHCP.start = "IPSTR, GOOD_IP2STR(wificonf->ap_dhcp_range.start_ip.addr));
	wifi_dbg("[WiFi] DHCP.end   = "IPSTR, GOOD_IP2STR(wificonf->ap_dhcp_range.end_ip.addr));
	wifi_dbg("[WiFi] DHCP.lease = %d minutes", wificonf->ap_dhcp_time);

	if (!wifi_softap_set_dhcps_lease(&wificonf->ap_dhcp_range)) {
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
		wifi_set_opmode_current(wificonf->opmode);
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
