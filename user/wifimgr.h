//
// Created by MightyPork on 2017/07/08.
// This module handles all WiFi configuration and is interfaced
// by the cgi_wifi functions.
//

#ifndef ESP_VT100_FIRMWARE_WIFI_MANAGER_H
#define ESP_VT100_FIRMWARE_WIFI_MANAGER_H

#include <esp8266.h>
#include "cgi_wifi.h"

#define SSID_LEN 32
#define PASSWORD_LEN 64

// Size designed for the wifi config structure
// Must be constant to avoid corrupting user config after upgrade
#define WIFICONF_SIZE 340

#define WIFICONF_VERSION 0

/**
 * A structure holding all configured WiFi parameters
 * and the active state.
 *
 * This block can be used eg. for WiFi config backup.
 */
typedef struct {
	WIFI_MODE opmode : 8;
	u8 tpw;

	// AP config
	u8 ap_channel;
	u8 ap_ssid[SSID_LEN];
	u8 ap_password[PASSWORD_LEN];
	bool ap_hidden;
	//
	u16 ap_dhcp_time; // in minutes
	struct dhcps_lease ap_dhcp_range;
	struct ip_info ap_addr;

	// Client config
	u8 sta_ssid[SSID_LEN];
	u8 sta_password[PASSWORD_LEN];
	bool sta_dhcp_enable;
	struct ip_info sta_addr;
	u8 config_version;
} WiFiConfigBundle;

typedef struct  {
	bool sta;
	bool ap;
} WiFiConfChangeFlags;

extern WiFiConfChangeFlags wifi_change_flags;

extern WiFiConfigBundle * const wificonf;

void wifimgr_restore_defaults(void);

void wifimgr_apply_settings(void);

#endif //ESP_VT100_FIRMWARE_WIFI_MANAGER_H
