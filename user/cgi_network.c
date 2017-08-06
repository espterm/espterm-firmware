/*
configuring the network settings
*/

#include <esp8266.h>
#include <httpdespfs.h>
#include "cgi_network.h"
#include "wifimgr.h"
#include "persist.h"
#include "helpers.h"

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

	char buff[50];

	char redir_url_buf[300];
	char *redir_url = redir_url_buf;
	redir_url += sprintf(redir_url, SET_REDIR_ERR);
	// we'll test if anything was printed by looking for \0 in failed_keys_buf

	if (connData->conn == NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
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

	if (redir_url_buf[strlen(SET_REDIR_ERR)] == 0) {
		// All was OK
		info("Set network params - success, applying in 1000 ms");

		// Settings are applied only if all was OK
		persist_store();

		// Delayed settings apply, so the response page has a chance to load.
		// If user connects via the Station IF, they may not even notice the connection reset.
		os_timer_disarm(&timer);
		os_timer_setfn(&timer, applyNetSettingsLaterCb, NULL);
		os_timer_arm(&timer, 1000, false);

		httpdRedirect(connData, SET_REDIR_SUC);
	} else {
		warn("Some WiFi settings did not validate, asking for correction");
		// Some errors, appended to the URL as ?err=
		httpdRedirect(connData, redir_url_buf);
	}
	return HTTPD_CGI_DONE;
}


//Template code for the WLAN page.
httpd_cgi_state ICACHE_FLASH_ATTR tplNetwork(HttpdConnData *connData, char *token, void **arg)
{
	char buff[100];
	u8 mac[6];

	if (token == NULL) {
		// We're done
		return HTTPD_CGI_DONE;
	}

	strcpy(buff, ""); // fallback

	if (streq(token, "ap_dhcp_time")) {
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
	else if (streq(token, "sta_dhcp_enable")) {
		sprintf(buff, "%d", wificonf->sta_dhcp_enable);
	}
	else if (streq(token, "sta_addr_ip")) {
		sprintf(buff, IPSTR, GOOD_IP2STR(wificonf->sta_addr.ip.addr));
	}
	else if (streq(token, "sta_addr_mask")) {
		sprintf(buff, IPSTR, GOOD_IP2STR(wificonf->sta_addr.netmask.addr));
	}
	else if (streq(token, "sta_addr_gw")) {
		sprintf(buff, IPSTR, GOOD_IP2STR(wificonf->sta_addr.gw.addr));
	}
	else if (streq(token, "sta_mac")) {
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
