//
// Created by MightyPork on 2017/07/08.
// This module handles all WiFi configuration and is interfaced
// by the cgi_wifi functions.
//

#ifndef ESP_VT100_FIRMWARE_WIFI_MANAGER_H
#define ESP_VT100_FIRMWARE_WIFI_MANAGER_H

#include <esp8266.h>
#include "cgi_wifi.h"

/**
 * A structure holding all configured WiFi parameters
 * and the active state.
 *
 * This block can be used eg. for WiFi config backup.
 */
typedef struct {
	WIFI_MODE opmode : 32;
	u8 sta_hostname[32];
	u32 tpw;

	// --- AP config ---
	u32 ap_channel; // 32 for alignment, needs 8
	u8 ap_ssid[32];
	u8 ap_password[32];
	u32 ap_hidden;
	u32 ap_dhcp_lease_time; // in minutes

	struct ip_info ap_ip;

	// --- Client config ---
	u8 sta_ssid[32];
	u8 sta_password[64];
	u32 sta_dhcp_enable;

	struct ip_info sta_ip;
} WiFiSettingsBlock;

extern WiFiSettingsBlock wificonf;

void wifimgr_restore_defaults(void);

void wifimgr_apply_settings(void);

#endif //ESP_VT100_FIRMWARE_WIFI_MANAGER_H
