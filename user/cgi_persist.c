/*
Cgi/template routines for configuring non-wifi settings
*/

#include <esp8266.h>
#include "cgi_persist.h"
#include "persist.h"
#include "helpers.h"
#include "cgi_logging.h"

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
