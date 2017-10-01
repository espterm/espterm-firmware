//
// Created by MightyPork on 2017/10/01.
//

#include <esp8266.h>
#include "cgi_d2d.h"
#include "apars_logging.h"
#include "version.h"
#include "ansi_parser_callbacks.h"
#include <httpclient.h>
#include <esp_utils.h>

#define D2D_TIMEOUT_MS 2000

#define D2D_HEADERS \
	"User-Agent: ESPTerm/"FIRMWARE_VERSION"\r\n" \
	"Content-Type: text/plain; charset=utf-8\r\n" \
	"Cache-Control: max-age=0\r\n"

struct d2d_request_opts {
	bool want_body;
	bool want_head;
	char *nonce;
};

static void ICACHE_FLASH_ATTR
requestCb(int http_status,
			   const char *response_headers,
			   const char *response_body,
			   size_t body_size,
			   void *userArg)
{
	if (userArg == NULL) return;
	struct d2d_request_opts *opts = userArg;

	char buff100[100];
	int len = 0;
	if (opts->want_head) len += strlen(response_headers);
	if (opts->want_body) len += body_size + (opts->want_head*2);
	char *bb = buff100;
	bb += sprintf(bb, "\x1b^h;%d;", http_status);
	const char *comma = "";

	if (opts->want_head) {
		bb += sprintf(bb, "%sH", comma);
		comma = ",";
	}

	if (opts->want_body) {
		bb += sprintf(bb, "%sB", comma);
		comma = ",";
	}

	if (opts->nonce) {
		bb += sprintf(bb, "%sN=%s", comma, opts->nonce);
		comma = ",";
	}

	if (opts->want_head || opts->want_body) {
		bb += sprintf(bb, "%sL=%d", comma, len);
		//comma = ",";
	}

	// semicolon only if more data is to be sent
	if (opts->want_head || opts->want_body)
		sprintf(bb, ";");

	apars_respond(buff100);

	dbg("Response %d, nonce %s", http_status, opts->nonce);
	dbg("Headers %s", response_headers);
	dbg("Body %s", response_body);

	// head and payload separated by \r\n\r\n (one \r\n is at the end of head - maybe)
	if (opts->want_head) {
		apars_respond(response_headers);
		if(opts->want_body) apars_respond("\r\n");
	}

	if(opts->want_body) {
		apars_respond(response_body);
	}

	apars_respond("\a");

	free(opts->nonce);
	free(userArg);
}

bool ICACHE_FLASH_ATTR
d2d_parse_command(char *msg)
{
	char buff40[40];
	char *p;

#define FIND_NEXT(target, delim) do { \
	p = strchr(msg, (delim)); \
	if (p == NULL) return false; \
	*p = '\0'; \
	(target) = msg; \
	msg = p + 1; \
} while(0) \

	if (strstarts(msg, "M;")) {
		// Send a esp-esp message
		msg += 2;
		const char *ip;
		FIND_NEXT(ip, ';');
		const char *payload = msg;

		ansi_dbg("D2D Tx,dest=%s,msg=%s", ip, payload);
		sprintf(buff40, "http://%s" D2D_MSG_ENDPOINT, ip);

		httpclient_args args;
		httpclient_args_init(&args);
		args.method = HTTPD_METHOD_POST;
		args.body = payload;
		args.headers = D2D_HEADERS;
		args.timeout = D2D_TIMEOUT_MS;
		args.url = buff40; // "escapes scope" warning - can ignore, strdup is used
		http_request(&args, NULL);
		return true;
	}
	else if (strstarts(msg, "H;")) {
		// Send a esp-esp message
		msg += 2;
		const char *method = NULL;
		const char *params = NULL;
		const char *nonce = NULL;
		const char *url = NULL;
		const char *payload = NULL;
		httpd_method methodNum = HTTPD_METHOD_GET;

		FIND_NEXT(method, ';');

		if (streq(method, "GET")) methodNum = HTTPD_METHOD_GET;
		else if (streq(method, "POST")) methodNum = HTTPD_METHOD_POST;
		else if (streq(method, "OPTIONS")) methodNum = HTTPD_METHOD_OPTIONS;
		else if (streq(method, "PUT")) methodNum = HTTPD_METHOD_PUT;
		else if (streq(method, "DELETE")) methodNum = HTTPD_METHOD_DELETE;
		else if (streq(method, "PATCH")) methodNum = HTTPD_METHOD_PATCH;
		else if (streq(method, "HEAD")) methodNum = HTTPD_METHOD_HEAD;
		else {
			warn("BAD METHOD: %s, using GET", method);
		}

		FIND_NEXT(params, ';');

		dbg("Method %s", method);
		dbg("Params %s", params);

		size_t max_len = HTTPCLIENT_DEF_MAX_LEN;
		int timeout = HTTPCLIENT_DEF_TIMEOUT_MS;
		bool want_body = 0;
		bool want_head = 0;
		bool no_resp = 0;

		do {
			p = strchr(params, ',');
			if (p != NULL) *p = '\0';
			const char *param = params;
			if (params[0] == 0) break; // no params

			if(streq(param, "H")) want_head = 1; // Return head
			else if(streq(param, "B")) want_body = 1; // Return body
			else if(streq(param, "X")) no_resp = 1; // X - no response, no callback
			else if(strstarts(param, "L=")) { // max length
				max_len = atoi(param+2);
			} else if(strstarts(param, "T=")) { // timeout
				timeout = atoi(param+2);
			} else if(strstarts(param, "N=")) { // Nonce
				nonce = param+2;
			} else {
				warn("BAD PARAM: %s", param);
				return false;
			}

			dbg("- param %s", params);

			if (p == NULL) break;
			params = p + 1;
		} while(1);

		p = strchr(msg, '\n');
		if (p != NULL) *p = '\0';
		url = msg;
		dbg("URL: %s", url);

		if (p != NULL)  {
			payload = p + 1;
			dbg("Payload: %s", payload);
		} else {
			payload = NULL;
		}

		httpclient_args args;
		httpclient_args_init(&args);
		args.method = methodNum;
		args.body = payload;
		args.headers = D2D_HEADERS;
		args.timeout = timeout;
		args.max_response_len = max_len;
		args.url = url;

		if (!no_resp) {
			struct d2d_request_opts *opts = malloc(sizeof(struct d2d_request_opts));
			opts->want_body = want_body;
			opts->want_head = want_head;
			opts->nonce = esp_strdup(nonce);
			args.userData = opts;
		}

		http_request(&args, no_resp ? NULL : requestCb);

		dbg("Done");
		return true;
	}

	return false;
}
