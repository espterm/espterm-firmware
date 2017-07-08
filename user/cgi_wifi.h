#ifndef CGIWIFI_H
#define CGIWIFI_H

#include "httpd.h"

/**
 * Convert IP hex to arguments for printf.
 * Library IP2STR(ip) does not work correctly due to unaligned memory access.
 */
#define GOOD_IP2STR(ip) ((ip)>>0)&0xff, ((ip)>>8)&0xff, ((ip)>>16)&0xff, ((ip)>>24)&0xff

httpd_cgi_state cgiWiFiScan(HttpdConnData *connData);
httpd_cgi_state cgiWiFiConnect(HttpdConnData *connData);
httpd_cgi_state cgiWiFiConnStatus(HttpdConnData *connData);
httpd_cgi_state cgiWiFiSetParams(HttpdConnData *connData);
httpd_cgi_state tplWlan(HttpdConnData *connData, char *token, void **arg);

// WiFi config options:
// - Persistent
//   - channel
//   - AP ssid
//   - opmode
//   - AP to connect to
// - Temporary
//   - sta_hostname (sta)
//   - tpw (ap, sta+ap?)
//   - dhcp_lt (ap, sta+ap)
//   - static IP
//   - static mask
//   - static gw
//   - dhcp enable or disable

#endif
