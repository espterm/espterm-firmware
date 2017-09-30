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

/** convert hex number to int */
static ICACHE_FLASH_ATTR u32
decodehex(const char *buf) {
	u32 n = 0;
	char c;
	while ((c = *buf++) != 0) {
		if (c >= '0' && c <= '9') c -= '0';
		else if (c >= 'a' && c <= 'f') c -= 'a'-10;
		else if (c >= 'A' && c <= 'F') c -= 'A'-10;
		else c = 0;
		n *= 16;
		n += c;
	}
	return n;
}

/**
 * Universal CGI endpoint to set Terminal params.
 */
httpd_cgi_state ICACHE_FLASH_ATTR
cgiTermCfgSetParams(HttpdConnData *connData)
{
	char buff[50];
	char redir_url_buf[100];
	int32 n, w, h;
	ScreenNotifyTopics topics = 0;

	bool shall_clear_screen = false;
	bool shall_init_uart = false;

	char *redir_url = redir_url_buf;
	redir_url += sprintf(redir_url, SET_REDIR_ERR);
	// we'll test if anything was printed by looking for \0 in failed_keys_buf

	SystemConfigBundle *sysconf_backup = malloc(sizeof(SystemConfigBundle));
	TerminalConfigBundle *termconf_backup = malloc(sizeof(TerminalConfigBundle));
	memcpy(sysconf_backup, sysconf, sizeof(SystemConfigBundle));
	memcpy(termconf_backup, termconf, sizeof(TerminalConfigBundle));

	if (connData->conn == NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	// width and height must always go together so we can do max size validation
	if (GET_ARG("term_width")) {
		do {
			cgi_dbg("Default screen width: %s", buff);
			w = atoi(buff);
			if (w < 1) {
				cgi_warn("Bad width: \"%s\"", buff);
				redir_url += sprintf(redir_url, "term_width,");
				break;
			}

			if (!GET_ARG("term_height")) {
				cgi_warn("Missing height arg!");
				// this wont happen normally when the form is used
				redir_url += sprintf(redir_url, "term_height,");
				break;
			}

			cgi_dbg("Default screen height: %s", buff);
			h = atoi(buff);
			if (h < 1) {
				cgi_warn("Bad height: \"%s\"", buff);
				redir_url += sprintf(redir_url, "term_height,");
				break;
			}

			if (w * h > MAX_SCREEN_SIZE) {
				cgi_warn("Bad dimensions: %d x %d (total %d)", w, h, w * h);
				redir_url += sprintf(redir_url, "term_width,term_height,");
				break;
			}

			if (termconf->width != w || termconf->height != h) {
				termconf->width = w;
				termconf->height = h;
				shall_clear_screen = true;
				topics |= TOPIC_CHANGE_SCREEN_OPTS | TOPIC_CHANGE_CONTENT_ALL;
			}
		} while (0);
	}

	if (GET_ARG("default_bg")) {
		cgi_dbg("Screen default BG: %s", buff);

		if (buff[0] == '#') {
			// decode hex
			n = decodehex(buff+1);
			n += 256;
		} else {
			n = atoi(buff);
		}

		if (termconf->default_bg != n) {
			termconf->default_bg = n; // this is current not sent through socket, no use to notify
			topics |= TOPIC_CHANGE_SCREEN_OPTS;
		}
	}

	if (GET_ARG("default_fg")) {
		cgi_dbg("Screen default FG: %s", buff);

		if (buff[0] == '#') {
			// decode hex
			n = decodehex(buff+1);
			n += 256;
		} else {
			n = atoi(buff);
		}

		if (termconf->default_fg != n) {
			termconf->default_fg = n; // this is current not sent through socket, no use to notify
			topics |= TOPIC_CHANGE_SCREEN_OPTS;
		}
	}

	if (GET_ARG("parser_tout_ms")) {
		cgi_dbg("Parser timeout: %s ms", buff);
		n = atoi(buff);
		if (n >= 0) {
			termconf->parser_tout_ms = n;
		} else {
			cgi_warn("Bad parser timeout %s", buff);
			redir_url += sprintf(redir_url, "parser_tout_ms,");
		}
	}

	if (GET_ARG("display_tout_ms")) {
		cgi_dbg("Display update idle timeout: %s ms", buff);
		n = atoi(buff);
		if (n >= 0) {
			termconf->display_tout_ms = n;
		} else {
			cgi_warn("Bad update timeout %s", buff);
			redir_url += sprintf(redir_url, "display_tout_ms,");
		}
	}

	if (GET_ARG("display_cooldown_ms")) {
		cgi_dbg("Display update cooldown: %s ms", buff);
		n = atoi(buff);
		if (n > 0) {
			termconf->display_cooldown_ms = n;
		} else {
			cgi_warn("Bad cooldown %s", buff);
			redir_url += sprintf(redir_url, "display_cooldown_ms,");
		}
	}

	if (GET_ARG("fn_alt_mode")) {
		cgi_dbg("FN alt mode: %s", buff);
		n = atoi(buff);
		termconf->fn_alt_mode = (bool)n;
		topics |= TOPIC_CHANGE_SCREEN_OPTS;
	}

	if (GET_ARG("want_all_fn")) {
		cgi_dbg("AllFN mode: %s", buff);
		n = atoi(buff);
		termconf->want_all_fn = (bool)n;
		topics |= TOPIC_CHANGE_SCREEN_OPTS;
	}

	if (GET_ARG("crlf_mode")) {
		cgi_dbg("CRLF mode: %s", buff);
		n = atoi(buff);
		termconf->crlf_mode = (bool)n;
		topics |= TOPIC_CHANGE_SCREEN_OPTS;
	}

	if (GET_ARG("show_buttons")) {
		cgi_dbg("Show buttons: %s", buff);
		n = atoi(buff);
		termconf->show_buttons = (bool)n;
		topics |= TOPIC_CHANGE_SCREEN_OPTS;
	}

	if (GET_ARG("show_config_links")) {
		cgi_dbg("Show config links: %s", buff);
		n = atoi(buff);
		termconf->show_config_links = (bool)n;
		topics |= TOPIC_CHANGE_SCREEN_OPTS;
	}

	if (GET_ARG("loopback")) {
		cgi_dbg("Loopback: %s", buff);
		n = atoi(buff);
		termconf->loopback = (bool)n;
		topics |= TOPIC_CHANGE_SCREEN_OPTS;
	}

	if (GET_ARG("debugbar")) {
		cgi_dbg("Debugbar: %s", buff);
		n = atoi(buff);
		termconf->debugbar = (bool)n;
		topics |= TOPIC_CHANGE_SCREEN_OPTS;
	}

	if (GET_ARG("allow_decopt_12")) {
		cgi_dbg("DECOPT 12: %s", buff);
		n = atoi(buff);
		termconf->allow_decopt_12 = (bool)n;
	}

	if (GET_ARG("theme")) {
		cgi_dbg("Screen color theme: %s", buff);
		n = atoi(buff);
		if (n >= 0) {
			termconf->theme = (u8) n;
			// this can't be notified, page must reload.
		} else {
			cgi_warn("Bad theme num: %s", buff);
			redir_url += sprintf(redir_url, "theme,");
			topics |= TOPIC_CHANGE_SCREEN_OPTS;
		}
	}

	if (GET_ARG("cursor_shape")) {
		cgi_dbg("Cursor shape: %s", buff);
		n = atoi(buff);
		if (n >= 0 && n <= 6 && n != 1) {
			termconf->cursor_shape = (enum CursorShape) n;
			topics |= TOPIC_CHANGE_SCREEN_OPTS;
		} else {
			cgi_warn("Bad cursor_shape num: %s", buff);
			redir_url += sprintf(redir_url, "cursor_shape,");
		}
	}

	if (GET_ARG("term_title")) {
		cgi_dbg("Terminal title default text: \"%s\"", buff);
		strncpy_safe(termconf->title, buff, 64); // ATTN those must match the values in
		topics |= TOPIC_CHANGE_TITLE;
	}

	for (int btn_i = 1; btn_i <= TERM_BTN_COUNT; btn_i++) {
		sprintf(buff, "btn%d", btn_i);
		if (GET_ARG(buff)) {
			cgi_dbg("Button%d default text: \"%s\"", btn_i, buff);
			strncpy_safe(termconf->btn[btn_i-1], buff, TERM_BTN_LEN);
			topics |= TOPIC_CHANGE_BUTTONS;
		}

		sprintf(buff, "bm%d", btn_i);
		if (GET_ARG(buff)) {
			cgi_dbg("Button%d message (ASCII): \"%s\"", btn_i, buff);

			// parse: comma,space or semicolon separated decimal values of ASCII codes
			char c;
			char *cp = buff;
			int char_i = 0;
			int acu = 0;
			bool lastsp = 1;
			char buff_bm[TERM_BTN_MSG_LEN];
			while ((c = *cp++) != 0) {
				if (c == ' ' || c == ',' || c == ';') {
					if(lastsp) continue;

					if (acu==0 || acu>255) {
						cgi_warn("Bad value! %d", acu);
						redir_url += sprintf(redir_url, "bm%d,", btn_i);
						break;
					}

					if (char_i >= TERM_BTN_MSG_LEN-1) {
						cgi_warn("Too long! %d", acu);
						redir_url += sprintf(redir_url, "bm%d,", btn_i);
						break;
					}

					cgi_dbg("acu %d", acu);
					buff_bm[char_i++] = (char)acu;

					// prepare for next char
					acu = 0;
					lastsp = 1;
				} else if (c>='0'&&c<='9') {
					lastsp = 0;
					acu *= 10;
					acu += c - '0';
				} else {
					cgi_warn("Bad syntax!");
					redir_url += sprintf(redir_url, "bm%d,", btn_i);
					break;
				}
			}
			if (lastsp && char_i == 0) {
				cgi_warn("Required!");
				redir_url += sprintf(redir_url, "bm%d,", btn_i);
			}
			if (!lastsp) {
				buff_bm[char_i++] = (char)acu;
			}
			buff_bm[char_i] = 0;
			cgi_dbg("%s, chari = %d", buff_bm, char_i);

			strncpy(termconf->btn_msg[btn_i-1], buff_bm, TERM_BTN_MSG_LEN);
		}
	}

	if (GET_ARG("uart_baud")) {
		cgi_dbg("Baud rate: %s", buff);
		int baud = atoi(buff);
		if (baud == BIT_RATE_300 ||
			baud == BIT_RATE_600 ||
			baud == BIT_RATE_1200 ||
			baud == BIT_RATE_2400 ||
			baud == BIT_RATE_4800 ||
			baud == BIT_RATE_9600 ||
			baud == BIT_RATE_19200 ||
			baud == BIT_RATE_38400 ||
			baud == BIT_RATE_57600 ||
			baud == BIT_RATE_74880 ||
			baud == BIT_RATE_115200 ||
			baud == BIT_RATE_230400 ||
			baud == BIT_RATE_460800 ||
			baud == BIT_RATE_921600 ||
			baud == BIT_RATE_1843200 ||
			baud == BIT_RATE_3686400) {
			sysconf->uart_baudrate = (u32) baud;
			shall_init_uart = true;
		} else {
			cgi_warn("Bad baud rate %s", buff);
			redir_url += sprintf(redir_url, "uart_baud,");
		}
	}

	if (GET_ARG("uart_parity")) {
		cgi_dbg("Parity: %s", buff);
		int parity = atoi(buff);
		if (parity >= 0 && parity <= 2) {
			sysconf->uart_parity = (UartParityMode) parity;
			shall_init_uart = true;
		} else {
			cgi_warn("Bad parity %s", buff);
			redir_url += sprintf(redir_url, "uart_parity,");
		}
	}

	if (GET_ARG("uart_stopbits")) {
		cgi_dbg("Stop bits: %s", buff);
		int stopbits = atoi(buff);
		if (stopbits >= 1 && stopbits <= 3) {
			sysconf->uart_stopbits = (UartStopBitsNum) stopbits;
			shall_init_uart = true;
		} else {
			cgi_warn("Bad stopbits %s", buff);
			redir_url += sprintf(redir_url, "uart_stopbits,");
		}
	}

	(void)redir_url;

	if (redir_url_buf[strlen(SET_REDIR_ERR)] == 0) {
		// All was OK
		info("Set term params - success, saving...");

		persist_store();

		if (shall_clear_screen) {
			terminal_apply_settings();
		} else {
			terminal_apply_settings_noclear();
		}

		if (shall_init_uart) {
			serialInit();
		}

		if (topics) screen_notifyChange(topics);

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
#define BUFLEN TERM_TITLE_LEN
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
	else if (streq(token, "parser_tout_ms")) {
		sprintf(buff, "%d", termconf->parser_tout_ms);
	}
	else if (streq(token, "display_tout_ms")) {
		sprintf(buff, "%d", termconf->display_tout_ms);
	}
	else if (streq(token, "display_cooldown_ms")) {
		sprintf(buff, "%d", termconf->display_cooldown_ms);
	}
	else if (streq(token, "fn_alt_mode")) {
		sprintf(buff, "%d", (int)termconf->fn_alt_mode);
	}
	else if (streq(token, "want_all_fn")) {
		sprintf(buff, "%d", (int)termconf->want_all_fn);
	}
	else if (streq(token, "crlf_mode")) {
		sprintf(buff, "%d", (int)termconf->crlf_mode);
	}
	else if (streq(token, "show_buttons")) {
		sprintf(buff, "%d", (int)termconf->show_buttons);
	}
	else if (streq(token, "show_config_links")) {
		sprintf(buff, "%d", (int)termconf->show_config_links);
	}
	else if (streq(token, "allow_decopt_12")) {
		sprintf(buff, "%d", (int)termconf->allow_decopt_12);
	}
	else if (streq(token, "loopback")) {
		sprintf(buff, "%d", (int)termconf->loopback);
	}
	else if (streq(token, "debugbar")) {
		sprintf(buff, "%d", (int)termconf->debugbar);
	}
	else if (streq(token, "theme")) {
		sprintf(buff, "%d", termconf->theme);
	}
	else if (streq(token, "default_bg")) {
		if (termconf->default_bg < 256) {
			sprintf(buff, "%d", termconf->default_bg);
		} else {
			sprintf(buff, "#%06X", termconf->default_bg - 256);
		}
	}
	else if (streq(token, "default_fg")) {
		if (termconf->default_fg < 256) {
			sprintf(buff, "%d", termconf->default_fg);
		} else {
			sprintf(buff, "#%06X", termconf->default_fg - 256);
		}
	}
	else if (streq(token, "cursor_shape")) {
		sprintf(buff, "%d", termconf->cursor_shape);
	}
	else if (streq(token, "term_title")) {
		strncpy_safe(buff, termconf->title, BUFLEN);
	}
	else if (streq(token, "uart_baud")) {
		sprintf(buff, "%d", sysconf->uart_baudrate);
	}
	else if (streq(token, "uart_parity")) {
		sprintf(buff, "%d", sysconf->uart_parity);
	}
	else if (streq(token, "uart_stopbits")) {
		sprintf(buff, "%d", sysconf->uart_stopbits);
	}
	else {
		for (int btn_i = 1; btn_i <= TERM_BTN_COUNT; btn_i++) {
			sprintf(buff2, "btn%d", btn_i);
			if (streq(token, buff2)) {
				strncpy_safe(buff, termconf->btn[btn_i-1], BUFLEN);
				break;
			}

			sprintf(buff2, "bm%d", btn_i);
			if (streq(token, buff2)) {
				char c;
				char *bp = buff;
				char *cp = termconf->btn_msg[btn_i-1];
				int n = 0;
				while((c = *cp++) != 0) {
					if(n>0) {
						*bp = ',';
						bp++;
					}
					bp += sprintf(bp, "%d", (int)c);
					n++;
				}
				break;
			}
		}
	}

	tplSend(connData, buff, -1);
	return HTTPD_CGI_DONE;
}
