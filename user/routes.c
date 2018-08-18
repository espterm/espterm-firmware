#include <esp8266.h>
#include <httpd.h>
#include <cgiwebsocket.h>
#include <httpdespfs.h>
#include <auth.h>

#include "routes.h"
#include "cgi_wifi.h"
#include "cgi_system.h"
#include "cgi_main.h"
#include "cgi_sockets.h"
#include "cgi_network.h"
#include "cgi_term_cfg.h"
#include "cgi_persist.h"
#include "syscfg.h"
#include "persist.h"
#include "api.h"
#include "cgi_d2d.h"

/**
 * Password for WiFi config
 */
static int ICACHE_FLASH_ATTR wifiPassFn(HttpdConnData *connData, int no, char *user, int userLen, char *pass, int passLen)
{
	if (no == 0) {
		os_strcpy(user, (const char *) sysconf->access_name);
		os_strcpy(pass, (const char *) sysconf->access_pw);
		return 1;
	}
	if (no == 1) {
		os_strcpy(user, "admin");
		os_strcpy(pass, persist.admin.pw);
		return 1;
	}
	return 0;
}

httpd_cgi_state ICACHE_FLASH_ATTR cgiOptionalPwLock(HttpdConnData *connData)
{
	bool protect = false;

	http_dbg("Route, %s, pwlock=%d", connData->url, sysconf->pwlock);

	switch (sysconf->pwlock) {
		case PWLOCK_ALL:
			protect = true;
			break;

		case PWLOCK_SETTINGS_NOTERM:
			protect = strstarts(connData->url, "/cfg") &&
				      !strstarts(connData->url, "/cfg/term");
			break;

		case PWLOCK_SETTINGS_ALL:
			protect = strstarts(connData->url, "/cfg");
			break;

		case PWLOCK_MENUS:
			protect = strstarts(connData->url, "/cfg") ||
					  strstarts(connData->url, "/about") ||
					  strstarts(connData->url, "/help");
			break;

		default:
		case PWLOCK_NONE:
			break;
	}

	// pages outside the normal scope

	if (sysconf->pwlock > PWLOCK_NONE) {
		if (strstarts(connData->url, "/api/v1/reboot")) protect = true;
	}

	if (sysconf->pwlock > PWLOCK_SETTINGS_NOTERM) {
		if (strstarts(connData->url, "/api/v1/clear")) protect = true;
	}

	if (sysconf->access_pw[0] == 0) {
		http_dbg("Access PW is nil, no protection.");
		protect = false;
	}

	if (protect) {
		http_dbg("Page is protected!");
		connData->cgiArg = wifiPassFn;
		return authBasic(connData);
	} else {
		http_dbg("Not protected");
		return HTTPD_CGI_NOTFOUND;
	}
}

/**
 * Application routes
 */
const HttpdBuiltInUrl routes[] ESP_CONST_DATA = {
	// redirect func for the captive portal
	ROUTE_CGI_ARG("*", cgiRedirectApClientToHostname, "esp-terminal.ap"),
	ROUTE_CGI("*", cgiOptionalPwLock),

	// --- Web pages ---
	ROUTE_TPL_FILE("/", tplScreen, "/term.tpl"),
	ROUTE_TPL_FILE("/about/?", tplAbout, "/about.tpl"),
	ROUTE_FILE("/help/?", "/help.html"),

	// --- Sockets ---
	ROUTE_WS(URL_WS_UPDATE, updateSockConnect),

	// --- System control ---

	// API endpoints
	ROUTE_CGI(API_REBOOT"/?", cgiResetDevice),
	ROUTE_CGI(API_PING"/?", cgiPing),
	ROUTE_CGI(API_CLEAR"/?", cgiResetScreen),
	ROUTE_CGI(API_D2D_MSG"/?", cgiD2DMessage),
	ROUTE_CGI(API_GPIO"/?", cgiGPIO),

	ROUTE_REDIRECT("/cfg/?", "/cfg/wifi"),

	ROUTE_TPL_FILE("/cfg/wifi/?", tplWlan, "/cfg_wifi.tpl"),
	ROUTE_FILE("/cfg/wifi/connecting/?", "/cfg_wifi_conn.html"),
	ROUTE_CGI("/cfg/wifi/scan", cgiWiFiScan),
	ROUTE_CGI("/cfg/wifi/connstatus", cgiWiFiConnStatus),
	ROUTE_CGI("/cfg/wifi/set", cgiWiFiSetParams),

	ROUTE_TPL_FILE("/cfg/network/?", tplNetwork, "/cfg_network.tpl"),
	ROUTE_CGI("/cfg/network/set", cgiNetworkSetParams),

	ROUTE_TPL_FILE("/cfg/term/?", tplTermCfg, "/cfg_term.tpl"),
	ROUTE_CGI("/cfg/term/set", cgiTermCfgSetParams),

	ROUTE_TPL_FILE("/cfg/system/?", tplSystemCfg, "/cfg_system.tpl"),
	ROUTE_CGI("/cfg/system/set", cgiSystemCfgSetParams),
	ROUTE_CGI("/cfg/system/export", cgiPersistExport),
	ROUTE_CGI("/cfg/system/import", cgiPersistImport),
	ROUTE_CGI("/cfg/system/write_defaults", cgiPersistWriteDefaults),
	ROUTE_CGI("/cfg/system/restore_defaults", cgiPersistRestoreDefaults),
	ROUTE_CGI("/cfg/system/restore_hard", cgiPersistRestoreHard),

	ROUTE_FILESYSTEM(),
	ROUTE_END(),
};

