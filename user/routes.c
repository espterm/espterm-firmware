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

#define WIFI_PROTECT 1
#define WIFI_AUTH_NAME "admin"
#define WIFI_AUTH_PASS "password"

#if WIFI_PROTECT
static int ICACHE_FLASH_ATTR wifiPassFn(HttpdConnData *connData, int no, char *user, int userLen, char *pass,
                                        int passLen);
#endif

/**
 * Application routes
 */
HttpdBuiltInUrl builtInUrls[] = {
	// redirect func for the captive portal
	ROUTE_CGI_ARG("*", cgiRedirectApClientToHostname, "esp8266.nonet"),

	// --- Web pages ---
	ROUTE_TPL_FILE("/", tplScreen, "term.tpl"),

	// --- Sockets ---
	ROUTE_WS(URL_WS_UPDATE, updateSockConnect),

	// --- System control ---
	ROUTE_CGI("/system/reset", cgiResetDevice),
	ROUTE_CGI("/system/ping", cgiPing),

	// --- WiFi config ---
#if WIFI_PROTECT
	ROUTE_AUTH("/wifi*", wifiPassFn),
#endif
	// TODO add those pages
//	ROUTE_REDIRECT("/wifi/", "/wifi"),
//	ROUTE_TPL_FILE("/wifi", tplWlan, "/pages/wifi.tpl"),

	ROUTE_CGI("/wifi/scan", cgiWiFiScan),
	ROUTE_CGI("/wifi/connect", cgiWiFiConnect),
	ROUTE_CGI("/wifi/connstatus", cgiWiFiConnStatus),
	ROUTE_CGI("/wifi/setmode", cgiWiFiSetMode),
	ROUTE_CGI("/wifi/setchannel", cgiWiFiSetChannel),

	ROUTE_FILESYSTEM(),
	ROUTE_END(),
};

// --- Wifi password protection ---

#if WIFI_PROTECT
/**
 * @brief BasicAuth name/password checking function.
 *
 * It's invoked by the authBasic() built-in route handler
 * until it returns 0. Each time it populates the provided
 * name and password buffer.
 *
 * @param connData : connection context
 * @param no       : user number (incremented each time it's called)
 * @param user     : user buffer
 * @param userLen  : user buffer size
 * @param pass     : password buffer
 * @param passLen  : password buffer size
 * @return 0 to end, 1 if more users are available.
 */
static int ICACHE_FLASH_ATTR wifiPassFn(HttpdConnData *connData, int no, char *user, int userLen, char *pass, int passLen)
{
	(void)connData;
	(void)userLen;
	(void)passLen;

	if (no == 0) {
		os_strcpy(user, WIFI_AUTH_NAME);
		os_strcpy(pass, WIFI_AUTH_PASS);
		return 1;
//Add more users this way. Check against incrementing no for each user added.
//  } else if (no==1) {
//      os_strcpy(user, "user1");
//      os_strcpy(pass, "something");
//      return 1;
	}
	return 0;
}
#endif
