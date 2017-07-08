//
// Created by MightyPork on 2017/07/08.
//

#include "wifi_manager.h"

WiFiSettingsBlock wificonf;

/**
 * Restore defaults in the WiFi config block.
 * This is to be called if the WiFi config is corrupted on startup,
 * before applying the config.
 */
void wifimgr_restore_defaults(void)
{
	u8 mac[6];
	wifi_get_macaddr(SOFTAP_IF, mac);

	wificonf.opmode = STATIONAP_MODE;
	wificonf.tpw = 20;
	wificonf.ap_channel = 1;
	sprintf((char *) wificonf.ap_ssid, "TERM-%02X%02X%02X", mac[3], mac[4], mac[5]);
	wificonf.ap_password[0] = 0; // PSK2 always if password is not null.
	wificonf.ap_dhcp_lease_time = 120;
	wificonf.ap_hidden = false;

	IP4_ADDR(&wificonf.ap_ip.ip, 192, 168, mac[5], 1);
	IP4_ADDR(&wificonf.ap_ip.netmask, 255, 255, 255, 0);
	IP4_ADDR(&wificonf.ap_ip.gw, 192, 168, mac[5], 1);

	// --- Client config ---
	wificonf.sta_ssid[0] = 0;
	wificonf.sta_password[0] = 0;
	//sprintf((char *) wificonf.sta_ssid, "Chlivek");
	//sprintf((char *) wificonf.sta_password, "prase chrochta");
	strcpy((char *) wificonf.sta_hostname, (char *) wificonf.ap_ssid); // use the same value for sta_hostname as AP name
	wificonf.sta_dhcp_enable = true;

	IP4_ADDR(&wificonf.sta_ip.ip, 192, 168, 0, (mac[5]==1?2:mac[5]));// avoid being the same as "default gw"
	IP4_ADDR(&wificonf.sta_ip.netmask, 255, 255, 255, 0);
	IP4_ADDR(&wificonf.sta_ip.gw, 192, 168, 0, 1);
}

/**
 * Event handler
 */
void wifimgr_event_cb(System_Event_t *event)
{
	switch (event->event) {
//		case EVENT_STAMODE_CONNECTED:
//		EVENT_STAMODE_DISCONNECTED,
//		EVENT_STAMODE_AUTHMODE_CHANGE,
//		EVENT_STAMODE_GOT_IP,
//		EVENT_STAMODE_DHCP_TIMEOUT,
//		EVENT_SOFTAPMODE_STACONNECTED,
//		EVENT_SOFTAPMODE_STADISCONNECTED,
//		EVENT_SOFTAPMODE_PROBEREQRECVED,
	}
}

static void configure_station(void)
{
	info("[WiFi] Configuring Station mode...");
	struct station_config conf;
	strcpy((char *) conf.ssid, (char *) wificonf.sta_ssid);
	strcpy((char *) conf.password, (char *) wificonf.sta_password);
	dbg("[WiFi] Connecting to \"%s\", password \"%s\"", conf.ssid, conf.password);
	conf.bssid_set = 0;
	conf.bssid[0] = 0;
	wifi_station_disconnect();
	wifi_station_set_config_current(&conf);
	dbg("[WiFi] Hostname = %s", wificonf.sta_hostname);
	wifi_station_set_hostname((char*)wificonf.sta_hostname);

	if (wificonf.sta_dhcp_enable) {
		dbg("[WiFi] Starting DHCP...");
		if (!wifi_station_dhcpc_start()) {
			error("[WiFi] DHCp failed to start!");
			return;
		}
	}
	else {
		info("[WiFi] Setting up static IP...");
		dbg("[WiFi] Client.ip   = "IPSTR, GOOD_IP2STR(wificonf.sta_ip.ip.addr));
		dbg("[WiFi] Client.mask = "IPSTR, GOOD_IP2STR(wificonf.sta_ip.netmask.addr));
		dbg("[WiFi] Client.gw   = "IPSTR, GOOD_IP2STR(wificonf.sta_ip.gw.addr));

		wifi_station_dhcpc_stop();
		// Load static IP config
		if (!wifi_set_ip_info(STATION_IF, &wificonf.sta_ip)) {
			error("[WiFi] Error setting static IP!");
			return;
		}
	}

	info("[WiFi] Trying to connect to AP...");
	wifi_station_connect();
}

static void configure_ap(void)
{
	bool suc;

	info("[WiFi] Configuring SoftAP mode...");
	// AP is enabled
	struct softap_config conf;
	conf.channel = wificonf.ap_channel;
	strcpy((char *) conf.ssid, (char *) wificonf.ap_ssid);
	strcpy((char *) conf.password, (char *) wificonf.ap_password);
	conf.authmode = (wificonf.ap_password[0] == 0 ? AUTH_OPEN : AUTH_WPA2_PSK);
	conf.ssid_len = strlen((char *) conf.ssid);
	conf.ssid_hidden = wificonf.ap_hidden;
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
	info("[WiFi] Configuring SoftAP local IP...");
	dbg("[WiFi] SoftAP.ip   = "IPSTR, GOOD_IP2STR(wificonf.ap_ip.ip.addr));
	dbg("[WiFi] SoftAP.mask = "IPSTR, GOOD_IP2STR(wificonf.ap_ip.netmask.addr));
	dbg("[WiFi] SoftAP.gw   = "IPSTR, GOOD_IP2STR(wificonf.ap_ip.gw.addr));

	wifi_softap_dhcps_stop();

	// Configure DHCP
	if (!wifi_set_ip_info(SOFTAP_IF, &wificonf.ap_ip)) {
		error("[WiFi] IP set fail!");
		return;
	}

	info("[WiFi] Configuring SoftAP DHCP server...");
	struct dhcps_lease dhcp_lease;
	struct ip_addr ip;
	ip.addr = wificonf.ap_ip.ip.addr;
	ip.addr = (ip.addr & 0x00FFFFFFUL) | ((((ip.addr >> 24) & 0xFF) + 99UL) << 24);
	dhcp_lease.start_ip.addr = ip.addr;
	ip.addr = (ip.addr & 0x00FFFFFFUL) | ((((ip.addr >> 24) & 0xFF) + 100UL) << 24);
	dhcp_lease.end_ip.addr = ip.addr;

	dbg("[WiFi] DHCP.start = "IPSTR, GOOD_IP2STR(dhcp_lease.start_ip.addr));
	dbg("[WiFi] DHCP.end   = "IPSTR, GOOD_IP2STR(dhcp_lease.end_ip.addr));
	dbg("[WiFi] DHCP.lease = %d minutes", wificonf.ap_dhcp_lease_time);

	if (!wifi_softap_set_dhcps_lease(&dhcp_lease)) {
		error("[WiFi] DHCP address range set fail!");
		return;
	}

	if (!wifi_softap_set_dhcps_lease_time(wificonf.ap_dhcp_lease_time)) {
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
void wifimgr_apply_settings(void)
{
	info("[WiFi] Initializing WiFi manager...");
//	wifi_set_event_handler_cb(wifimgr_event_cb);

	// Force wifi cycle
	dbg("[WiFi] WiFi reset to apply new settings");
	wifi_set_opmode(NULL_MODE);
	wifi_set_opmode(wificonf.opmode);

	// Configure the client
	if (wificonf.opmode == STATIONAP_MODE || wificonf.opmode == STATION_MODE) {
		configure_station();
	}

	// Configure the AP
	if (wificonf.opmode == STATIONAP_MODE || wificonf.opmode == SOFTAP_MODE) {
		configure_ap();
	}
}
