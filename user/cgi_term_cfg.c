/*
Cgi/template routines for configuring non-wifi settings
*/

#include <esp8266.h>
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

	if (GET_ARG("term_title")) {
		dbg("Terminal title default text: \"%s\"", buff);
		strncpy_safe(termconf->title, buff, 64); // ATTN those must match the values in
	}

	if (GET_ARG("btn1")) {
		dbg("Button1 default text: \"%s\"", buff);
		strncpy_safe(termconf->btn1, buff, 10);
	}

	if (GET_ARG("btn2")) {
		dbg("Button1 default text: \"%s\"", buff);
		strncpy_safe(termconf->btn2, buff, 10);
	}

	if (GET_ARG("btn3")) {
		dbg("Button1 default text: \"%s\"", buff);
		strncpy_safe(termconf->btn3, buff, 10);
	}

	if (GET_ARG("btn4")) {
		dbg("Button1 default text: \"%s\"", buff);
		strncpy_safe(termconf->btn4, buff, 10);
	}

	if (GET_ARG("btn5")) {
		dbg("Button1 default text: \"%s\"", buff);
		strncpy_safe(termconf->btn5, buff, 10);
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
	else if (streq(token, "default_bg")) {
		sprintf(buff, "%d", termconf->default_bg);
	}
	else if (streq(token, "default_fg")) {
		sprintf(buff, "%d", termconf->default_fg);
	}
	else if (streq(token, "term_title")) {
		strncpy_safe(buff, termconf->title, BUFLEN);
	}
	else if (streq(token, "btn1")) {
		strncpy_safe(buff, termconf->btn1, BUFLEN);
	}
	else if (streq(token, "btn2")) {
		strncpy_safe(buff, termconf->btn2, BUFLEN);
	}
	else if (streq(token, "btn3")) {
		strncpy_safe(buff, termconf->btn3, BUFLEN);
	}
	else if (streq(token, "btn4")) {
		strncpy_safe(buff, termconf->btn4, BUFLEN);
	}
	else if (streq(token, "btn5")) {
		strncpy_safe(buff, termconf->btn5, BUFLEN);
	}

	httpdSend(connData, buff, -1);
	return HTTPD_CGI_DONE;
}
