/*
Cgi/template routines for configuring non-wifi settings
*/

#include <esp8266.h>
#include "cgi_persist.h"
#include "persist.h"
#include "helpers.h"
#include "cgi_logging.h"
#include "version.h"
#include "screen.h"
#include "config_xmacros.h"
#include "ini_parser.h"

#define SET_REDIR_SUC "/cfg/system"

static bool ICACHE_FLASH_ATTR
verify_admin_pw(const char *pw)
{
	return streq(pw, persist.admin.pw);
}

httpd_cgi_state ICACHE_FLASH_ATTR
cgiPersistWriteDefaults(HttpdConnData *connData)
{
	char buff[PASSWORD_LEN];

	if (connData->conn == NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	// width and height must always go together so we can do max size validation
	if (GET_ARG("pw")) {
		cgi_dbg("Entered password for admin: %s", buff);
		if (verify_admin_pw(buff)) {
			cgi_dbg("pw is OK");

			persist_set_as_default();

			httpdRedirect(connData, SET_REDIR_SUC "?msg=Default%20settings%20updated.");
			return HTTPD_CGI_DONE;
		}
		// if pw failed, show the same error as if it's wrong
	}

	httpdRedirect(connData, SET_REDIR_SUC "?err=Password"); // this will show in the "validation errors" box
	return HTTPD_CGI_DONE;
}


httpd_cgi_state ICACHE_FLASH_ATTR
cgiPersistRestoreDefaults(HttpdConnData *connData)
{
	if (connData->conn == NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	persist_restore_default();

	httpdRedirect(connData, SET_REDIR_SUC "?msg=All%20settings%20restored%20to%20saved%20defaults.");
	return HTTPD_CGI_DONE;
}

httpd_cgi_state ICACHE_FLASH_ATTR
cgiPersistRestoreHard(HttpdConnData *connData)
{
	if (connData->conn == NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	// this only changes live settings (and persists it)
	// Defaults are not changed.
	persist_load_hard_default();

	httpdRedirect(connData, SET_REDIR_SUC "?msg=All%20settings%20restored%20to%20factory%20defaults.");
	return HTTPD_CGI_DONE;
}



#define httpdSend_orDie(conn, data, len) do { if (!httpdSend((conn), (data), (len))) return false; } while (0)

/* encode for double-quoted string */
int ICACHE_FLASH_ATTR httpdSend_dblquot(HttpdConnData *conn, const char *data, int len)
{
	int start = 0, end = 0;
	char c;
	if (conn->conn==NULL) return 0;
	if (len < 0) len = (int) strlen(data);
	if (len==0) return 0;

	for (end = 0; end < len; end++) {
		c = data[end];
		if (c == 0) {
			// we found EOS
			break;
		}

		if (c == '"' || c == '\'' || c == '\\' || c == '\n' || c == '\r' || c == '\x1b') {
			if (start < end) httpdSend_orDie(conn, data + start, end - start);
			start = end + 1;
		}

		if (c == '"') httpdSend_orDie(conn, "\\\"", 2);
		else if (c == '\'') httpdSend_orDie(conn, "\\'", 2);
		else if (c == '\\') httpdSend_orDie(conn, "\\\\", 2);
		else if (c == '\n') httpdSend_orDie(conn, "\\n", 2);
		else if (c == '\r') httpdSend_orDie(conn, "\\r", 2);
		else if (c == '\x1b') httpdSend_orDie(conn, "\\e", 2);
	}

	if (start < end) httpdSend_orDie(conn, data + start, end - start);
	return 1;
}


/**
 * Export settings to INI
 *
 * @param connData
 * @return status
 */
httpd_cgi_state ICACHE_FLASH_ATTR
cgiPersistExport(HttpdConnData *connData)
{
	char buff[256];
	u8 mac[6];

	int step = (int) connData->cgiData;

	if (connData->conn == NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	if (step == 0) {
		wifi_get_macaddr(SOFTAP_IF, mac);
		sprintf(buff, "attachment; filename=ESPTerm_%02X%02X%02X_%s.ini",
				mac[3], mac[4],	mac[5], FW_VERSION);

		httpdStartResponse(connData, 200);
		httpdHeader(connData, "Content-Disposition", buff);
		httpdHeader(connData, "Content-Type", "text/plain");
		httpdEndHeaders(connData);

		sprintf(buff, "# == ESPTerm config export ==\r\n"
			"# Device: %02X%02X%02X - %s\r\n"
			"# Version: %s\r\n",
				mac[3], mac[4], mac[5],
				termconf->title,
				FW_VERSION);

		httpdSend(connData, buff, -1);
	}

	// do not export SSID if unchanged - embeds unique ID that should
	// not be overwritten in target.

	char defSSID[20];
	sprintf(defSSID, "TERM-%02X%02X%02X", mac[3], mac[4], mac[5]);

	bool quoted;
#define X(type, name, suffix, deref, xget, xset, xsarg, xnotify, allow) \
        do { if (allow) { \
            xget(buff, deref XSTRUCT->name); \
			if (streq(#name, "ap_ssid") && streq(buff, defSSID)) break; \
			\
            quoted = false; \
            quoted |= streq(#type, "char"); \
            quoted |= streq(#type, "uchar"); \
			if (strstarts(#name, "bm")) quoted=false; \
			\
            httpdSend(connData, "\r\n"#name " = ", -1); \
            if (quoted) { \
				httpdSend(connData, "\"", 1); \
                httpdSend_dblquot(connData, buff, -1); \
                httpdSend(connData, "\"", 1); \
            } else { \
                httpdSend(connData, buff, -1); \
            } \
		} } while(0);

#define admin 1
#define tpl 1
	if (step == 1) {
		httpdSend(connData, "\r\n[system]", -1);
#define XSTRUCT sysconf
		XTABLE_SYSCONF
#undef XSTRUCT
		httpdSend(connData, "\r\n", -1);
	}

	if (step == 2) {
		httpdSend(connData, "\r\n[wifi]", -1);
#define XSTRUCT wificonf
		XTABLE_WIFICONF
#undef XSTRUCT
		httpdSend(connData, "\r\n", -1);
	}

	if (step == 3) {
		httpdSend(connData, "\r\n[terminal]", -1);
#define XSTRUCT termconf
		XTABLE_TERMCONF
#undef XSTRUCT
		httpdSend(connData, "\r\n", -1);

		return HTTPD_CGI_DONE;
	}

#undef X
	connData->cgiData = (void *) (step + 1);
	return HTTPD_CGI_MORE;
}


void iniCb(const char *section, const char *key, const char *value, void *userData)
{
	dbg(">>> SET: [%s] %s = %s", section, key, value);
}


httpd_cgi_state ICACHE_FLASH_ATTR
postRecvHdl(HttpdConnData *connData, char *data, int len)
{
	ini_parse(data, (size_t) len);
	ini_parse_end();
	return HTTPD_CGI_DONE;
}

/**
 * Import settings from INI
 *
 * @param connData
 * @return status
 */
httpd_cgi_state ICACHE_FLASH_ATTR
cgiPersistImport(HttpdConnData *connData)
{
	if (connData->conn == NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	httpdStartResponse(connData, 200);
	httpdHeader(connData, "Content-Type", "text/plain");
	httpdEndHeaders(connData);

	char *start = strstr(connData->post->buff, "\r\n\r\n");
	if (start == NULL) {
		error("Malformed attachment POST!");
		goto end;
	}

	ini_parse_begin(iniCb, NULL);
	ini_parse(start, (size_t) connData->post->buffLen - (start - connData->post->buff));

	connData->recvHdl = postRecvHdl;

end:
	// TODO redirect
	return HTTPD_CGI_DONE;
}
