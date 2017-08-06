/*
Cgi/template routines for configuring non-wifi settings
*/

#include <esp8266.h>
#include <httpdespfs.h>
#include "cgi_term_cfg.h"
#include "persist.h"
#include "screen.h"
#include "helpers.h"

#define SET_REDIR_SUC "/cfg/term"
#define SET_REDIR_ERR SET_REDIR_SUC"?err="

/**
 * Universal CGI endpoint to set Terminal params.
 */
httpd_cgi_state ICACHE_FLASH_ATTR
cgiTermCfgSetParams(HttpdConnData *connData)
{
	char buff[50];

	char redir_url_buf[300];
	char *redir_url = redir_url_buf;
	redir_url += sprintf(redir_url, SET_REDIR_ERR);
	// we'll test if anything was printed by looking for \0 in failed_keys_buf

	if (connData->conn == NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	// width and height must always go together so we can do max size validation
	if (GET_ARG("term_width")) {
		dbg("Default screen width: %s", buff);
		int w = atoi(buff);
		if (w > 1) {
			if (GET_ARG("term_height")) {
				dbg("Default screen height: %s", buff);
				int h = atoi(buff);
				if (h > 1) {
					if (w * h <= MAX_SCREEN_SIZE) {
						termconf->width = w;
						termconf->height = h;
					} else {
						warn("Bad dimensions: %d x %d (total %d)", w, h, w*h);
						redir_url += sprintf(redir_url, "term_width,term_height,");
					}
				} else {
					warn("Bad height: \"%s\"", buff);
					redir_url += sprintf(redir_url, "term_width,");
				}
			} else {
				warn("Missing height arg!");
				// this wont happen normally when the form is used
				redir_url += sprintf(redir_url, "term_width,term_height,");
			}
		} else {
			warn("Bad width: \"%s\"", buff);
			redir_url += sprintf(redir_url, "term_width,");
		}
	}

	if (GET_ARG("default_bg")) {
		dbg("Screen default BG: %s", buff);
		int color = atoi(buff);
		if (color >= 0 && color < 16) {
			termconf->default_bg = (u8) color;
		} else {
			warn("Bad color %s", buff);
			redir_url += sprintf(redir_url, "default_bg,");
		}
	}

	if (GET_ARG("default_fg")) {
		dbg("Screen default FG: %s", buff);
		int color = atoi(buff);
		if (color >= 0 && color < 16) {
			termconf->default_fg = (u8) color;
		} else {
			warn("Bad color %s", buff);
			redir_url += sprintf(redir_url, "default_fg,");
		}
	}

	if (GET_ARG("theme")) {
		dbg("Screen color theme: %s", buff);
		int theme = atoi(buff);
		if (theme >= 0 && theme <= 5) { // ALWAYS ADJUST WHEN ADDING NEW THEME!
			termconf->theme = (u8) theme;
		} else {
			warn("Bad theme num: %s", buff);
			redir_url += sprintf(redir_url, "theme,");
		}
	}

	if (GET_ARG("term_title")) {
		dbg("Terminal title default text: \"%s\"", buff);
		strncpy_safe(termconf->title, buff, 64); // ATTN those must match the values in
	}

	for (int i = 1; i <= 5; i++) {
		sprintf(buff, "btn%d", i);
		if (GET_ARG(buff)) {
			dbg("Button%d default text: \"%s\"", i, buff);
			strncpy_safe(termconf->btn[i-1], buff, TERM_BTN_LEN);
		}
	}

	if (redir_url_buf[strlen(SET_REDIR_ERR)] == 0) {
		// All was OK
		info("Set term params - success, saving...");

		terminal_apply_settings();
		persist_store();

		httpdRedirect(connData, SET_REDIR_SUC);
	} else {
		warn("Some settings did not validate, asking for correction");
		// Some errors, appended to the URL as ?err=
		httpdRedirect(connData, redir_url_buf);
	}
	return HTTPD_CGI_DONE;
}


httpd_cgi_state ICACHE_FLASH_ATTR
tplTermCfg(HttpdConnData *connData, char *token, void **arg)
{
#define BUFLEN 100
	char buff[BUFLEN];
	char buff2[10];

	if (token == NULL) {
		// We're done
		return HTTPD_CGI_DONE;
	}

	strcpy(buff, ""); // fallback

	if (streq(token, "term_width")) {
		sprintf(buff, "%d", termconf->width);
	}
	else if (streq(token, "term_height")) {
		sprintf(buff, "%d", termconf->height);
	}
	else if (streq(token, "theme")) {
		sprintf(buff, "%d", termconf->theme);
	}
	else if (streq(token, "default_bg")) {
		sprintf(buff, "%d", termconf->default_bg);
	}
	else if (streq(token, "default_fg")) {
		sprintf(buff, "%d", termconf->default_fg);
	}
	else if (streq(token, "term_title")) {
		strncpy_safe(buff, termconf->title, BUFLEN);
	}
	else {
		for (int i = 1; i <= 5; i++) {
			sprintf(buff2, "btn%d", i);
			if (streq(token, buff2)) {
				strncpy_safe(buff, termconf->btn[i-1], BUFLEN);
				break;
			}
		}
	}

	tplSend(connData, buff, -1);
	return HTTPD_CGI_DONE;
}
