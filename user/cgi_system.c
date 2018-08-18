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

	// flags for the template builder
	bool uart_changed = false; //!< this is set in uart notify, for use in terminal settings (dummy here)
	bool admin = false;
	const bool tpl = false; // this optionally disables some fields
	do {
		// Check admin PW
		if (GET_ARG("pw")) {
			if (!streq(buff, persist.admin.pw)) {
				warn("Bad admin pw!");
				redir_url += sprintf(redir_url, "pw,");
				break; // Abort
			} else {
				admin = true;
			}
		}

		// Changing admin PW
		if (admin && GET_ARG("admin_pw")) {
			if (strlen(buff)) {
				cgi_dbg("admin_pw: %s", buff);

				strcpy(buff2, buff);
				if (!GET_ARG("admin_pw2")) {
					cgi_warn("Missing repeated admin_pw %s", buff);
					redir_url += sprintf(redir_url, "admin_pw2,");
					break; // Abort
				}

				if (!streq(buff, buff2)) {
					cgi_warn("Bad repeated admin_pw %s", buff);
					redir_url += sprintf(redir_url, "admin_pw2,");
					break; // Abort
				}

				if (strlen(buff) >= 64) {
					cgi_warn("Too long admin_pw %s", buff);
					redir_url += sprintf(redir_url, "admin_pw,");
					break; // Abort
				}

				cgi_dbg("Changing admin PW!");
				strncpy(persist.admin.pw, buff, 64);

				break; // this is the only field in this form
			}
		}

		// Reject filled but unconfirmed access PW
		if (admin && GET_ARG("access_pw")) {
			if (strlen(buff)) {
				cgi_dbg("access_pw: %s", buff);

				strcpy(buff2, buff);
				if (!GET_ARG("access_pw2")) {
					cgi_warn("Missing repeated access_pw %s", buff);
					redir_url += sprintf(redir_url, "access_pw2,");
					break; // Abort
				}

				if (!streq(buff, buff2)) {
					cgi_warn("Bad repeated access_pw %s", buff);
					redir_url += sprintf(redir_url, "access_pw2,");
					break; // Abort
				}
			}
		}

		// Settings in the system config block
#define XSTRUCT sysconf
#define X XSET_CGI_FUNC
		XTABLE_SYSCONF
#undef X
#undef XSTRUCT

	} while (0);

	(void)redir_url;
	(void)uart_changed; // unused

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

	const bool admin = false;
	const bool tpl=true;

#define XSTRUCT sysconf
#define X XGET_CGI_FUNC
	XTABLE_SYSCONF
#undef X
#undef XSTRUCT

	if (streq(token, "def_access_name")) {
		sprintf(buff, "%s", DEF_ACCESS_NAME);
	}
	else if (streq(token, "def_access_pw")) {
		sprintf(buff, "%s", DEF_ACCESS_PW);
	}
	else if (streq(token, "def_admin_pw")) {
		sprintf(buff, "%s", DEFAULT_ADMIN_PW);
	}

	tplSend(connData, buff, -1);
	return HTTPD_CGI_DONE;
}


static ETSTimer tmrPulse;

static void ICACHE_FLASH_ATTR tmrPulseCb(void *arg)
{
	u32 mask = sysconf->gpio2_conf==GPIOCONF_OFF ? 0x30 : 0x34;
	u32 argu = (u32) arg;
	u32 set_on = argu & mask;
	u32 set_off = (argu >> 16) & mask;
	gpio_output_set(set_off, set_on, 0, 0); // the args are swapped here to achieve the opposite
}

/**
 * API to set GPIOs
 *
 * @param connData
 * @return
 */
httpd_cgi_state ICACHE_FLASH_ATTR cgiGPIO(HttpdConnData *connData)
{
#define BUFLEN 100
	char buff[BUFLEN];

	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	bool set = 0;
	u32 inputs = gpio_input_get();

	// TODO pulse option

	// args are do2, do4, do5. Values: 0 - off, 1 - on, -1 - toggle

	s8 set_d2 = -1, set_d4 = -1, set_d5 = -1;

	if (sysconf->gpio2_conf == GPIOCONF_OUT_START_0 || sysconf->gpio2_conf == GPIOCONF_OUT_START_1) {
		if (GET_ARG("do2")) {
			if (buff[0] == 't') {
				set = ((inputs&(1<<2)) == 0);
			} else {
				set = buff[0] == '1';
			}
			set_d2 = set;
			gpio_output_set((uint32) (set << 2), (uint32) ((!set) << 2), 0, 0);
		}
	}

	if (sysconf->gpio4_conf == GPIOCONF_OUT_START_0 || sysconf->gpio4_conf == GPIOCONF_OUT_START_1) {
		if (GET_ARG("do4")) {
			if (buff[0] == 't') {
				set = ((inputs&(1<<4)) == 0);
			} else {
				set = buff[0] == '1';
			}
			set_d4 = set;
			gpio_output_set((uint32) (set << 4), (uint32) ((!set) << 4), 0, 0);
		}
	}

	if (sysconf->gpio5_conf == GPIOCONF_OUT_START_0 || sysconf->gpio5_conf == GPIOCONF_OUT_START_1) {
		if (GET_ARG("do5")) {
			if (buff[0] == 't') {
				set = ((inputs&(1<<5)) == 0);
			} else {
				set = buff[0] == '1';
			}
			set_d5 = set;
			gpio_output_set((uint32) (set << 5), (uint32) ((!set) << 5), 0, 0);
		}
	}

	if (GET_ARG("pulse")) {
		int duration = atoi(buff);

		if (duration > 0) {
			u32 cmd = 0;
			if (set_d2 != -1) {
				if (set_d2) {
					cmd |= 1<<2;
				} else {
					cmd |= 1<<(16+2);
				}
			}

			if (set_d4 != -1) {
				if (set_d4) {
					cmd |= 1<<4;
				} else {
					cmd |= 1<<(16+4);
				}
			}

			if (set_d5 != -1) {
				if (set_d5) {
					cmd |= 1<<5;
				} else {
					cmd |= 1<<(16+5);
				}
			}

			os_timer_disarm(&tmrPulse);
			os_timer_setfn(&tmrPulse, tmrPulseCb, (void*) cmd);
			os_timer_arm(&tmrPulse, duration, false);
		}
	}

	httpdStartResponse(connData, 200);
	httpdHeader(connData, "Content-Type", "application/json");
	httpdEndHeaders(connData);

	// refresh inputs
	inputs = gpio_input_get();

	sprintf(buff, "{\"io2\":%d,\"io4\":%d,\"io5\":%d}",
			((inputs&(1<<2)) != 0),
			((inputs&(1<<4)) != 0),
			((inputs&(1<<5)) != 0)
		);

	httpdSend(connData, buff, -1);

	return HTTPD_CGI_DONE;
}
