#include <esp8266.h>
#include <httpd.h>
#include <cgiwebsocket.h>
#include <httpdespfs.h>
#include <cgiwifi.h>
#include <auth.h>

#include "routes.h"

#include "cgi_reset.h"
#include "cgi_ping.h"
#include "cgi_main.h"
#include "cgi_sockets.h"

#define WIFI_PROTECT 0
#define WIFI_AUTH_NAME "wifi"
#define WIFI_AUTH_PASS "nicitel"

static int wifiPassFn(HttpdConnData *connData, int no, char *user, int userLen, char *pass, int passLen);

/**
 * Application routes
 */
HttpdBuiltInUrl routes[] = {
	// redirect func for the captive portal
	ROUTE_CGI_ARG("*", cgiRedirectApClientToHostname, "esp-remote-term.ap"),

	// --- Web pages ---
	ROUTE_TPL_FILE("/", tplScreen, "/term.tpl"),

	// --- Sockets ---
	ROUTE_WS(URL_WS_UPDATE, updateSockConnect),

	// --- System control ---
	ROUTE_CGI("/system/reset", cgiResetDevice),
	ROUTE_CGI("/system/ping", cgiPing),

	// --- WiFi config ---
#if WIFI_PROTECT
	ROUTE_AUTH("/wifi*", wifiPassFn),
#endif
	ROUTE_REDIRECT("/wifi/", "/wifi"),
	ROUTE_TPL_FILE("/wifi", tplWlan, "/wifi.tpl"),

	ROUTE_CGI("/wifi/scan", cgiWiFiScan),
	ROUTE_CGI("/wifi/connect", cgiWiFiConnect),
	ROUTE_CGI("/wifi/connstatus", cgiWiFiConnStatus),
	ROUTE_CGI("/wifi/setmode", cgiWiFiSetMode),
	ROUTE_CGI("/wifi/setchannel", cgiWiFiSetChannel),
	ROUTE_CGI("/wifi/setname", cgiWiFiSetSSID),

	ROUTE_FILESYSTEM(),
	ROUTE_END(),
};

// --- Wifi password protection ---

/**
 * Password for WiFi config
 */
static int ICACHE_FLASH_ATTR wifiPassFn(HttpdConnData *connData, int no, char *user, int userLen, char *pass, int passLen)
{
	if (no == 0) {
		os_strcpy(user, WIFI_AUTH_NAME);
		os_strcpy(pass, WIFI_AUTH_PASS);
		return 1;
	}
	return 0;
}
