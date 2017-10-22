/*
Cgi/template routines for configuring non-wifi settings
*/

#include <esp8266.h>
#include <httpdespfs.h>
#include "cgi_term_cfg.h"
#include "persist.h"
#include "screen.h"
#include "helpers.h"
#include "cgi_logging.h"
#include "uart_driver.h"
#include "serial.h"

#define SET_REDIR_SUC "/cfg/term"
#define SET_REDIR_ERR SET_REDIR_SUC"?err="

/**
 * Universal CGI endpoint to set Terminal params.
 */
httpd_cgi_state ICACHE_FLASH_ATTR
cgiTermCfgSetParams(HttpdConnData *connData)
{
	char buff[50];
	char redir_url_buf[100];
	ScreenNotifyTopics topics = 0;

	char *redir_url = redir_url_buf;
	redir_url += sprintf(redir_url, SET_REDIR_ERR);
	// we'll test if anything was printed by looking for \0 in redir_url

	SystemConfigBundle *sysconf_backup = malloc(sizeof(SystemConfigBundle));
	TerminalConfigBundle *termconf_backup = malloc(sizeof(TerminalConfigBundle));
	memcpy(sysconf_backup, sysconf, sizeof(SystemConfigBundle));
	memcpy(termconf_backup, termconf, sizeof(TerminalConfigBundle));

	if (connData->conn == NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

#define XSTRUCT termconf
#define X XSET_CGI_FUNC
	XTABLE_TERMCONF
#undef X
#undef XSTRUCT

	// width and height must always go together so we can do max size validation
	u32 siz = termconf->width*termconf->height;
	if (siz == 0 || siz > MAX_SCREEN_SIZE) {
		cgi_warn("Bad dimensions: %d x %d (total %d)", termconf->width, termconf->height, termconf->width*termconf->height);
		redir_url += sprintf(redir_url, "term_width,term_height,");
	}

	// Settings from SysConf - UART
	bool uart_changed = false;
	// restrict what keys are allowed
#define admin 0
#define tpl 0
#define XSTRUCT sysconf
#define X XSET_CGI_FUNC
	XTABLE_SYSCONF
#undef X
#undef XSTRUCT

	(void)redir_url;

	if (redir_url_buf[strlen(SET_REDIR_ERR)] == 0) {
		// All was OK
		info("Set term params - success, saving...");

		persist_store();

		bool shall_clear_screen = false;
		shall_clear_screen |= (termconf_backup->width != termconf->width);
		shall_clear_screen |= (termconf_backup->height != termconf->height);
		shall_clear_screen |= (termconf_backup->ascii_debug != termconf->ascii_debug);

		if (shall_clear_screen) {
			terminal_apply_settings();
		} else {
			terminal_apply_settings_noclear();
		}

		if (uart_changed) {
			sysconf_apply_settings();
		}

		// Trigger a full reload
		screen_notifyChange(TOPIC_INITIAL);

		httpdRedirect(connData, SET_REDIR_SUC "?msg=Settings%20saved%20and%20applied.");
	} else {
		cgi_warn("Some settings did not validate, asking for correction");

		memcpy(sysconf, sysconf_backup, sizeof(SystemConfigBundle));
		memcpy(termconf, termconf_backup, sizeof(TerminalConfigBundle));

		// Some errors, appended to the URL as ?err=
		httpdRedirect(connData, redir_url_buf);
	}

	free(sysconf_backup);
	free(termconf_backup);
	return HTTPD_CGI_DONE;
}


httpd_cgi_state ICACHE_FLASH_ATTR
tplTermCfg(HttpdConnData *connData, char *token, void **arg)
{
#define BUFLEN 100 // large enough for backdrop
	char buff[BUFLEN];

	if (token == NULL) {
		// We're done
		return HTTPD_CGI_DONE;
	}

	strcpy(buff, ""); // fallback

#define XSTRUCT termconf
#define X XGET_CGI_FUNC
	XTABLE_TERMCONF
#undef X
#undef XSTRUCT

	tplSend(connData, buff, -1);
	return HTTPD_CGI_DONE;
}
