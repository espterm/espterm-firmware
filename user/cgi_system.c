#include <esp8266.h>
#include <httpd.h>
#include <helpers.h>
#include <httpdespfs.h>

#include "cgi_system.h"
#include "persist.h"
#include "syscfg.h"
#include "uart_driver.h"
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

	httpdSend(connData, "system reset\n", -1);

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

	httpdSend(connData, "pong\n", -1);

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
		} else {
			cgi_warn("Bad stopbits %s", buff);
			redir_url += sprintf(redir_url, "uart_stopbits,");
		}
	}

	if (GET_ARG("security")) {
		cgi_dbg("*** Security config! ***");

		if (GET_ARG("pw")) {
			if (streq(buff, persist.admin.pw)) {
				// authenticated OK
				do {
					if (GET_ARG("pwlock")) {
						cgi_dbg("pwlock: %s", buff);
						int pwlock = atoi(buff);
						if (pwlock >= 0 && pwlock < PWLOCK_MAX) {
							sysconf->pwlock = (enum pwlock) pwlock;
						}
						else {
							cgi_warn("Bad pwlock %s", buff);
							redir_url += sprintf(redir_url, "pwlock,");
							break;
						}
					}

					if (GET_ARG("access_pw")) {
						cgi_dbg("access_pw: %s", buff);

						strcpy(buff2, buff);
						if (GET_ARG("access_pw2")) {
							cgi_dbg("access_pw2: %s", buff);

							if (streq(buff, buff2)) {
								cgi_dbg("Changing access PW!!!");
								strncpy(sysconf->access_pw, buff, 64);
							} else {
								cgi_warn("Bad repeated access_pw %s", buff);
								redir_url += sprintf(redir_url, "access_pw2,");
							}
						} else {
							cgi_warn("Missing access_pw %s", buff);
							redir_url += sprintf(redir_url, "access_pw2,");
						}

						break; // access pw and admin pw are in separate forms
					}

					if (GET_ARG("admin_pw")) {
						cgi_dbg("admin_pw: %s", buff);

						strcpy(buff2, buff);
						if (GET_ARG("admin_pw2")) {
							cgi_dbg("admin_pw2: %s", buff);

							if (streq(buff, buff2)) {
								cgi_dbg("Changing admin PW!!!");
								strncpy(persist.admin.pw, buff, 64);
							} else {
								cgi_warn("Bad repeated admin_pw %s", buff);
								redir_url += sprintf(redir_url, "admin_pw2,");
							}
						} else {
							cgi_warn("Missing admin_pw %s", buff);
							redir_url += sprintf(redir_url, "admin_pw2,");
						}

						break;
					}
				} while(0);
			} else {
				warn("Bad admin pw!");
				redir_url += sprintf(redir_url, "pw,");
			}
		} else {
			warn("Missing admin pw!");
			redir_url += sprintf(redir_url, "pw,");
		}
	}

	if (redir_url_buf[strlen(SET_REDIR_ERR)] == 0) {
		// All was OK
		cgi_info("Set system params - success, saving...");

		sysconf_apply_settings();
		persist_store();

		httpdRedirect(connData, SET_REDIR_SUC);
	} else {
		cgi_warn("Some settings did not validate, asking for correction");
		// Some errors, appended to the URL as ?err=
		httpdRedirect(connData, redir_url_buf);
	}
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

	if (streq(token, "uart_baud")) {
		sprintf(buff, "%d", sysconf->uart_baudrate);
	}
	else if (streq(token, "uart_parity")) {
		sprintf(buff, "%d", sysconf->uart_parity);
	}
	else if (streq(token, "uart_stopbits")) {
		sprintf(buff, "%d", sysconf->uart_stopbits);
	}
	else if (streq(token, "pwlock")) {
		sprintf(buff, "%d", sysconf->pwlock);
	}

	tplSend(connData, buff, -1);
	return HTTPD_CGI_DONE;
}
