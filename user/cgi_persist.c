/*
Cgi/template routines for configuring non-wifi settings
*/

#include <esp8266.h>
#include "cgi_persist.h"
#include "persist.h"
#include "helpers.h"

#define SET_REDIR_SUC "/cfg/admin"

static bool ICACHE_FLASH_ATTR
verify_admin_pw(const char *pw)
{
	// This is not really for security, but to prevent someone who
	// shouldn't touch those settings from fucking it up.
	return streq(pw, STR(ADMIN_PASSWORD)); // the PW comes from the makefile
}

httpd_cgi_state ICACHE_FLASH_ATTR
cgiPersistWriteDefaults(HttpdConnData *connData)
{
	char buff[50];

	if (connData->conn == NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	// width and height must always go together so we can do max size validation
	if (GET_ARG("pw")) {
		dbg("Entered password for admin: %s", buff);
		if (verify_admin_pw(buff)) {
			dbg("pw is OK");

			persist_set_as_default();

			httpdRedirect(connData, SET_REDIR_SUC);
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

	httpdRedirect(connData, SET_REDIR_SUC);
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

	httpdRedirect(connData, SET_REDIR_SUC);
	return HTTPD_CGI_DONE;
}
