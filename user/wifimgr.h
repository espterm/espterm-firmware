//
// Created by MightyPork on 2017/07/08.
// This module handles all WiFi configuration and is interfaced
// by the cgi_wifi functions.
//

#ifndef ESP_VT100_FIRMWARE_WIFI_MANAGER_H
#define ESP_VT100_FIRMWARE_WIFI_MANAGER_H

#include <esp8266.h>
#include "config_xmacros.h"
#include "cgi_wifi.h"

#define SSID_LEN 32
#define PASSWORD_LEN 64

// Size designed for the wifi config structure
// Must be constant to avoid corrupting user config after upgrade
#define WIFICONF_SIZE 340

#define WIFICONF_VERSION 0

#define wifimgr_notify_ap() { wifi_change_flags.ap = true; }
#define wifimgr_notify_sta() { wifi_change_flags.ap = true; }

//....Type................Name..Suffix...............Deref..XGET.........Cast..XSET.........................NOTIFY................Allow
// Deref is used to pass the field to xget. Cast is used to convert the &'d field to what xset wants (needed for static arrays)
#define XTABLE_WIFI \
	X(u8,              opmode, /**/,                  /**/, xget_dec,      /**/, xset_wifi_opmode, NULL,      /**/,              1) \
	\
	X(u8,              tpw, /**/,                     /**/, xget_dec,      /**/, xset_wifi_tpw, NULL,         wifimgr_notify_ap(),  1) \
	X(u8,              ap_channel, /**/,              /**/, xget_dec,      /**/, xset_wifi_ap_channel, NULL,  wifimgr_notify_ap(),  1) \
	X(u8,              ap_ssid, [SSID_LEN],           /**/, xget_ustring,  /**/, xset_wifi_ssid, 1,         wifimgr_notify_ap(),  1) \
	X(u8,              ap_password, [PASSWORD_LEN],   /**/, xget_ustring,  /**/, xset_wifi_pwd, NULL,       wifimgr_notify_ap(),  1) \
	X(bool,            ap_hidden, /**/,               /**/, xget_bool,     /**/, xset_bool, NULL,             wifimgr_notify_ap(),  1) \
	\
	X(u16,             ap_dhcp_time, /**/,            /**/, xget_dec,      /**/, xset_wifi_lease_time, NULL,  wifimgr_notify_ap(),  1) \
	X(u32,             unused1, /**/,                 /**/, xget_dummy,    /**/, xset_dummy, NULL,            /**/,                 0) \
	X(struct ip_addr,  ap_dhcp_start, /**/,           &,    xget_ip,       /**/, xset_ip, NULL,               wifimgr_notify_ap(),  1) \
	X(struct ip_addr,  ap_dhcp_end, /**/,             &,    xget_ip,       /**/, xset_ip, NULL,               wifimgr_notify_ap(),  1) \
	\
	X(struct ip_addr,  ap_addr_ip, /**/,              &,    xget_ip,       /**/, xset_ip, NULL,               wifimgr_notify_ap(),  1) \
	X(struct ip_addr,  ap_addr_mask, /**/,            &,    xget_ip,       /**/, xset_ip, NULL,               wifimgr_notify_ap(),  1) \
	\
	\
	X(u32,             unused2, /**/,                 /**/, xget_dummy,    /**/, xset_dummy, NULL,            /**/,                 0) \
	X(u8,              sta_ssid, [SSID_LEN],          /**/, xget_ustring,  /**/, xset_wifi_ssid, 0,         wifimgr_notify_sta(), 1) \
	X(u8,              sta_password, [PASSWORD_LEN],  /**/, xget_ustring,  /**/, xset_wifi_pwd, NULL,       wifimgr_notify_sta(), 1) \
	X(bool,            sta_dhcp_enable, /**/,         /**/, xget_bool,     /**/, xset_bool, NULL,             wifimgr_notify_sta(), 1) \
	\
	X(struct ip_addr,  sta_addr_ip, /**/,             &,    xget_ip,       /**/, xset_ip, NULL,               wifimgr_notify_sta(), 1) \
	X(struct ip_addr,  sta_addr_mask, /**/,           &,    xget_ip,       /**/, xset_ip, NULL,               wifimgr_notify_sta(), 1) \
	X(struct ip_addr,  sta_addr_gw, /**/,             &,    xget_ip,       /**/, xset_ip, NULL,               wifimgr_notify_sta(), 1) \
	\
	\
	X(u8,              config_version, /**/,          /**/, xget_dec,      /**/, xset_u8, NULL,               /**/,                 1)

// unused1 - replaces 'enabled' bit from old dhcps_lease struct
// unused2 - gap after 'ap_gw' which isn't used and doesn't make sense

/**
 * A structure holding all configured WiFi parameters
 * and the active state.
 *
 * This block can be used eg. for WiFi config backup.
 */
typedef struct {
#define X XSTRUCT_FIELD
	XTABLE_WIFI
#undef X
} WiFiConfigBundle;

typedef struct  {
	bool sta;
	bool ap;
} WiFiConfChangeFlags;

extern WiFiConfChangeFlags wifi_change_flags;

extern WiFiConfigBundle * const wificonf;

void wifimgr_restore_defaults(void);

void wifimgr_apply_settings(void);

int getStaIpAsString(char *buffer);

enum xset_result xset_wifi_lease_time(const char *name, u16 *field, const char *buff, const void *arg);
enum xset_result xset_wifi_opmode(const char *name, u8 *field, const char *buff, const void *arg);
enum xset_result xset_wifi_tpw(const char *name, u8 *field, const char *buff, const void *arg);
enum xset_result xset_wifi_ap_channel(const char *name, u8 *field, const char *buff, const void *arg);
enum xset_result xset_wifi_ssid(const char *name, uchar *field, const char *buff, const void *arg);
enum xset_result xset_wifi_pwd(const char *name, uchar *field, const char *buff, const void *arg);

#if DEBUG_WIFI
#define wifi_warn warn
#define wifi_dbg dbg
#define wifi_info info
#else
#define wifi_warn(fmt, ...)
#define wifi_dbg(fmt, ...)
#define wifi_info(fmt, ...)
#endif

#endif //ESP_VT100_FIRMWARE_WIFI_MANAGER_H
