#include <esp8266.h>
#include <httpd.h>
#include <helpers.h>
#include <httpdespfs.h>

#include "cgi_system.h"
#include "persist.h"
#include "syscfg.h"
#include "ansi_parser.h"
#include "cgi_logging.h"

#define SET_REDIR_SUC "/cfg/system"
#define SET_REDIR_ERR SET_REDIR_SUC"?err="

static ETSTimer tmr;

static void ICACHE_FLASH_ATTR tmrCb(void *arg)
{
	system_restart();
}

httpd_cgi_state ICACHE_FLASH_ATTR cgiResetScreen(HttpdConnData *connData)
{
	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	info("--- User request to reset screen! ---");
	// this copies termconf to scratch and also resets the screen
	terminal_apply_settings();
	ansi_parser_reset();

	httpdRedirect(connData, "/");
	return HTTPD_CGI_DONE;
}

httpd_cgi_state ICACHE_FLASH_ATTR cgiResetDevice(HttpdConnData *connData)
{
	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	httpdStartResponse(connData, 200);
	httpdHeader(connData, "Content-Type", "text/plain");
	httpdEndHeaders(connData);

	os_timer_disarm(&tmr);
	os_timer_setfn(&tmr, tmrCb, NULL);
	os_timer_arm(&tmr, 100, false);

	httpdSend(connData, "system reset\r\n", -1);

	return HTTPD_CGI_DONE;
}

httpd_cgi_state ICACHE_FLASH_ATTR cgiPing(HttpdConnData *connData)
{
	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	httpdStartResponse(connData, 200);
	httpdHeader(connData, "Content-Type", "text/plain");
	httpdEndHeaders(connData);

	httpdSend(connData, "pong\r\n", -1);

	return HTTPD_CGI_DONE;
}

/**
 * Universal CGI endpoint to set Terminal params.
 */
httpd_cgi_state ICACHE_FLASH_ATTR
cgiSystemCfgSetParams(HttpdConnData *connData)
{
	char buff[65];
	char buff2[65];
	char redir_url_buf[100];

	char *redir_url = redir_url_buf;
	redir_url += sprintf(redir_url, SET_REDIR_ERR);
	// we'll test if anything was printed by looking for \0 in failed_keys_buf

	if (connData->conn == NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	AdminConfigBlock *admin_backup = malloc(sizeof(AdminConfigBlock));
	SystemConfigBundle *sysconf_backup = malloc(sizeof(SystemConfigBundle));
	memcpy(admin_backup, &persist.admin, sizeof(AdminConfigBlock));
	memcpy(sysconf_backup, sysconf, sizeof(SystemConfigBundle));

	do {
		if (!GET_ARG("pw")) {
			break; // if no PW in GET, not trying to configure anything protected
		}

		if (!streq(buff, persist.admin.pw)) {
			warn("Bad admin pw!");
			redir_url += sprintf(redir_url, "pw,");
			break;
		}

		// authenticated OK
		if (GET_ARG("pwlock")) {
			cgi_dbg("pwlock: %s", buff);
			int pwlock = atoi(buff);
			if (pwlock < 0 || pwlock >= PWLOCK_MAX) {
				cgi_warn("Bad pwlock %s", buff);
				redir_url += sprintf(redir_url, "pwlock,");
				break;
			}

			sysconf->pwlock = (enum pwlock) pwlock;
		}

		if (GET_ARG("access_pw")) {
			cgi_dbg("access_pw: %s", buff);

			if (strlen(buff)) {
				strcpy(buff2, buff);
				if (!GET_ARG("access_pw2")) {
					cgi_warn("Missing repeated access_pw %s", buff);
					redir_url += sprintf(redir_url, "access_pw2,");
					break;
				}

				if (!streq(buff, buff2)) {
					cgi_warn("Bad repeated access_pw %s", buff);
					redir_url += sprintf(redir_url, "access_pw2,");
					break;
				}

				if (strlen(buff) >= 64) {
					cgi_warn("Too long access_pw %s", buff);
					redir_url += sprintf(redir_url, "access_pw,");
					break;
				}

				cgi_dbg("Changing access PW!");
				strncpy(sysconf->access_pw, buff, 64);
			}
		}

		if (GET_ARG("access_name")) {
			cgi_dbg("access_name: %s", buff);

			if (!strlen(buff) || strlen(buff) >= 32) {
				cgi_warn("Too long access_name %s", buff);
				redir_url += sprintf(redir_url, "access_name,");
				break;
			}

			strncpy(sysconf->access_name, buff, 32);
		}

		if (GET_ARG("admin_pw")) {
			cgi_dbg("admin_pw: %s", buff);

			if (strlen(buff)) {
				strcpy(buff2, buff);
				if (!GET_ARG("admin_pw2")) {
					cgi_warn("Missing repeated admin_pw %s", buff);
					redir_url += sprintf(redir_url, "admin_pw2,");
					break;
				}

				if (!streq(buff, buff2)) {
					cgi_warn("Bad repeated admin_pw %s", buff);
					redir_url += sprintf(redir_url, "admin_pw2,");
					break;
				}

				if (strlen(buff) >= 64) {
					cgi_warn("Too long admin_pw %s", buff);
					redir_url += sprintf(redir_url, "admin_pw,");
					break;
				}

				cgi_dbg("Changing admin PW!");
				strncpy(persist.admin.pw, buff, 64);
			}
		}
	} while (0);

	if (GET_ARG("overclock")) {
		cgi_dbg("overclock = %s", buff);
		int enable = atoi(buff);
		if (sysconf->overclock != enable) {
			sysconf->overclock = (bool)enable;
		}
	}

	(void)redir_url;

	if (redir_url_buf[strlen(SET_REDIR_ERR)] == 0) {
		// All was OK
		cgi_info("Set system params - success, saving...");

		sysconf_apply_settings();
		persist_store();

		httpdRedirect(connData, SET_REDIR_SUC "?msg=Settings%20saved%20and%20applied.");
	} else {
		cgi_warn("Some settings did not validate, asking for correction");

		// revert any possible changes
		memcpy(&persist.admin, admin_backup, sizeof(AdminConfigBlock));
		memcpy(sysconf, sysconf_backup, sizeof(SystemConfigBundle));

		// Some errors, appended to the URL as ?err=
		httpdRedirect(connData, redir_url_buf);
	}

	free(admin_backup);
	free(sysconf_backup);
	return HTTPD_CGI_DONE;
}


httpd_cgi_state ICACHE_FLASH_ATTR
tplSystemCfg(HttpdConnData *connData, char *token, void **arg)
{
#define BUFLEN 100
	char buff[BUFLEN];

	if (token == NULL) {
		// We're done
		return HTTPD_CGI_DONE;
	}

	strcpy(buff, ""); // fallback

	if (streq(token, "pwlock")) {
		sprintf(buff, "%d", sysconf->pwlock);
	}
	else if (streq(token, "access_name")) {
		sprintf(buff, "%s", sysconf->access_name);
	}
	else if (streq(token, "def_access_name")) {
		sprintf(buff, "%s", DEF_ACCESS_NAME);
	}
	else if (streq(token, "def_access_pw")) {
		sprintf(buff, "%s", DEF_ACCESS_PW);
	}
	else if (streq(token, "def_admin_pw")) {
		sprintf(buff, "%s", DEFAULT_ADMIN_PW);
	}
	else if (streq(token, "overclock")) {
		sprintf(buff, "%d", sysconf->overclock);
	}

	tplSend(connData, buff, -1);
	return HTTPD_CGI_DONE;
}
