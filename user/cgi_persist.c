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
#define SET_REDIR_ERR SET_REDIR_SUC"?err="

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

// -------------- Export --------------

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


// -------------- IMPORT --------------

struct IniUpload {
	TerminalConfigBundle *term_backup;
	WiFiConfigBundle *wifi_backup;
	SystemConfigBundle *sys_backup;
	bool term_ok;
	bool wifi_ok;
	bool sys_ok;
	bool term_any;
	bool wifi_any;
	bool sys_any;
};


static void ICACHE_FLASH_ATTR iniCb(const char *section, const char *key, const char *value, void *userData)
{
	HttpdConnData *connData = (HttpdConnData *)userData;
	struct IniUpload *state;
	if (!connData || !connData->cgiData) {
		error("userData or state is NULL!");
		return;
	}
	state = connData->cgiData;

//	cgi_dbg("%s.%s = %s", section, key, value);

/** used for INI */
#define X(type, name, suffix, deref, xget, xset, xsarg, xnotify, allow) \
	if (streq(#name, key)) { \
		found = true; \
		type *_p = (type *) &XSTRUCT->name; \
		enum xset_result res = xset(#name, _p, value, (const void*) (xsarg)); \
		if (res == XSET_SET) { changed = true; xnotify; } \
		else if (res == XSET_FAIL) { ok = false; } \
		break; \
	}

	// notify flag, unused here
	bool uart_changed = false;

	bool found = false;
	bool ok = true;
	bool changed = false;

	if (streq(section, "terminal")) {
		do {
#define XSTRUCT termconf
			XTABLE_TERMCONF
#undef XSTRUCT
		} while (0);

		if (found) {
			if (!ok) state->term_ok = false;
			state->term_any |= changed;
		}
	}
	else if (streq(section, "wifi")) {
		do {
#define XSTRUCT wificonf
			XTABLE_WIFICONF
#undef XSTRUCT
		} while (0);

		if (found) {
			if (!ok) state->wifi_ok = false;
			state->wifi_any |= changed;
		}
	}
	else if (streq(section, "system")) {
		do {
#define XSTRUCT sysconf
			XTABLE_SYSCONF
#undef XSTRUCT
		} while (0);

		if (found) {
			if (!ok) state->sys_ok = false;
			state->sys_any |= changed;
		}
	}

	if (!found) cgi_warn("Unknown key %s.%s!", section, key);

#undef X
}

static void ICACHE_FLASH_ATTR
freeIniUploadStruct(HttpdConnData *connData)
{
	cgi_dbg("Free struct...");
	struct IniUpload *state;
	if (connData && connData->cgiData) {
		state = connData->cgiData;
		if (state->sys_backup != NULL) free(state->sys_backup);
		if (state->wifi_backup != NULL) free(state->wifi_backup);
		if (state->term_backup != NULL) free(state->term_backup);
		free(state);
		connData->cgiData = NULL;
	}
}

static httpd_cgi_state ICACHE_FLASH_ATTR
postRecvHdl(HttpdConnData *connData, char *data, int len)
{
	struct IniUpload *state = connData->cgiData;
	if (!state) return HTTPD_CGI_DONE;

	cgi_dbg("INI parse - postRecvHdl");

	if (len>0) {
		// Discard the boundary marker (there is only one)
		char *bdr = connData->post->multipartBoundary;
		char *foundat = strstr(data, bdr);
		if (foundat != NULL) {
			*foundat = '#'; // make it a comment
		}

		ini_parse(data, (size_t) len);
	}

	cgi_dbg("Closing parser");
	ini_parse_end();

	cgi_dbg("INI parse - end.");

	// abort if bad screen size
	bool tooLarge = (termconf->width*termconf->height > MAX_SCREEN_SIZE);
	state->term_ok &= !tooLarge;
	if (tooLarge) cgi_warn("Bad term screen size!");

	bool suc = state->term_ok && state->wifi_ok && state->sys_ok;
	bool any = state->term_any || state->wifi_any || state->sys_any;

	cgi_dbg("Evaluating results...");

	if (!state->term_ok) cgi_warn("Terminal settings rejected.");
	if (!state->wifi_ok) cgi_warn("WiFi settings rejected.");
	if (!state->sys_ok) cgi_warn("System settings rejected.");

	if (!suc) {
		cgi_warn("Some validation failed, reverting all!");
		memcpy(termconf, state->term_backup, sizeof(TerminalConfigBundle));
		memcpy(wificonf, state->wifi_backup, sizeof(WiFiConfigBundle));
		memcpy(sysconf, state->sys_backup, sizeof(SystemConfigBundle));
	}
	else {
		if (state->term_any) {
			cgi_dbg("Applying terminal settings");
			terminal_apply_settings();
		}

		if (state->sys_any) {
			cgi_dbg("Applying system  settings");
			sysconf_apply_settings();
		}

		if (state->wifi_any) {
			cgi_dbg("Applying WiFi settings (scheduling...)");
			wifimgr_apply_settings_later(2000);
		}

		if (any) {
			cgi_dbg("Persisting results");
			persist_store();
		} else {
			cgi_warn("Nothing written.");
		}
	}

	cgi_dbg("Redirect");
	char buff[100];
	char *b = buff;
	if (suc) {
		if (any) {
			httpdRedirect(connData, SET_REDIR_SUC"?msg=Settings%20loaded%20and%20applied.");
		} else {
			httpdRedirect(connData, SET_REDIR_SUC"?msg=No%20settings%20changed.");
		}
	} else {
		b += sprintf(b, SET_REDIR_SUC"?errmsg=Errors%%20in:%%20");
		bool comma = false;
		if (!state->sys_ok) {
			b += sprintf(b, "System%%20config");
			comma = true;
		}
		if (!state->wifi_ok) {
			if (comma) b += sprintf(b, ",%%20");
			b += sprintf(b, "WiFi%%20config");
		}
		if (!state->term_ok) {
			if (comma) b += sprintf(b, ",%%20");
			b += sprintf(b, "Terminal%%20config");
		}
		httpdRedirect(connData, buff);
	}

	// Clean up.
	freeIniUploadStruct(connData);
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
	struct IniUpload *state = NULL;

	if (connData->conn == NULL) {
		//Connection aborted. Clean up.
		freeIniUploadStruct(connData);
		return HTTPD_CGI_DONE;
	}

	char *start = strstr(connData->post->buff, "\r\n\r\n");
	if (start == NULL) {
		error("Malformed attachment POST!");

		httpdStartResponse(connData, 400);
		httpdHeader(connData, "Content-Type", "text/plain");
		httpdEndHeaders(connData);
		httpdSend(connData, "Bad format.", -1);
		return HTTPD_CGI_DONE;
	}

	cgi_info("Starting INI parser for uploaded file...");

	state = malloc(sizeof(struct IniUpload));
	if (!state) {
		error("state struct alloc fail");
		return HTTPD_CGI_DONE;
	}
	state->sys_backup = NULL;
	state->wifi_backup = NULL;
	state->term_backup = NULL;
	connData->cgiData = state;

	cgi_dbg("Allocating backup buffers");
	state->sys_backup = malloc(sizeof(SystemConfigBundle));
	state->wifi_backup = malloc(sizeof(WiFiConfigBundle));
	state->term_backup = malloc(sizeof(TerminalConfigBundle));

	cgi_dbg("Copying orig data");
	memcpy(state->sys_backup, sysconf, sizeof(SystemConfigBundle));
	memcpy(state->wifi_backup, wificonf, sizeof(WiFiConfigBundle));
	memcpy(state->term_backup, termconf, sizeof(TerminalConfigBundle));

	state->sys_ok = true;
	state->wifi_ok = true;
	state->term_ok = true;

	state->sys_any = false;
	state->wifi_any = false;
	state->term_any = false;

	cgi_dbg("Parser starts!");
	ini_parse_begin(iniCb, connData);

	size_t datalen = (size_t) connData->post->buffLen - (start - connData->post->buff);
	ini_parse(start, datalen);

	connData->recvHdl = postRecvHdl;

	// special case for too short ini
	int bytes_remain = connData->post->len;
	bytes_remain -= connData->post->buffLen;

	if (bytes_remain <= 0) {
		return connData->recvHdl(connData, NULL, 0);
	}

	// continues in recvHdl
	return HTTPD_CGI_MORE;
}
