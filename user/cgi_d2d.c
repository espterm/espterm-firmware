//
// Created by MightyPork on 2017/10/01.
//

#include <esp8266.h>
#include "cgi_d2d.h"
#include "version.h"
#include "ansi_parser_callbacks.h"
#include "api.h"
#include "uart_driver.h"
#include <httpclient.h>
#include <esp_utils.h>

#define D2D_TIMEOUT_MS 2000

#define D2D_HEADERS \
	"User-Agent: ESPTerm "VERSION_STRING" like curl wget HTTPie\r\n" \
	"Content-Type: text/plain; charset=utf-8\r\n" \
	"Accept-Encoding: identity\r\n" \
	"Accept-Charset: utf-8\r\n" \
	"Accept: text/*, application/json\r\n" \
	"Cache-Control: no-cache,private,max-age=0\r\n"

struct d2d_request_opts {
	bool want_body;
	bool want_head;
	size_t max_result_len;
	char *nonce;
};

volatile bool request_pending = false;

// NOTE! We bypass the async buffer here - used for user input and
// responses to queries {apars_respond()}. In rare situations this could
// lead to a race condition and mixing two different messages
static inline void ICACHE_FLASH_ATTR sendResponseToUART(const char *str)
{
	UART_WriteString(UART0, str, UART_TIMEOUT_US);
}

static void ICACHE_FLASH_ATTR
requestNoopCb(int http_status,
			   char *response_headers,
			   char *response_body,
			   size_t body_size,
			   void *userArg)
{
	request_pending = false;
	if (userArg != NULL) free(userArg);
}

static void ICACHE_FLASH_ATTR
requestCb(int http_status,
			   char *response_headers,
			   char *response_body,
			   size_t body_size,
			   void *userArg)
{
	if (userArg == NULL) {
		request_pending = false;
		return;
	}

	struct d2d_request_opts *opts = userArg;

	// ensure positive - would be hard to parse
	if (http_status < 0) http_status = -http_status;

	char buff100[100];
	int len = 0;
	size_t headers_size = strlen(response_headers);

	if (opts->want_head) len += headers_size;
	if (opts->want_body) len += body_size + (opts->want_head*2);
	if (opts->max_result_len > 0 && len > opts->max_result_len)
		len = (int) opts->max_result_len;

	d2d_info("Rx HTTP response, code %d, len %d, nonce \"%s\"", http_status, len, opts->nonce?opts->nonce:"");

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
	if (opts->want_head || opts->want_body)	sprintf(bb, ";");

	sendResponseToUART(buff100);

	//d2d_dbg("Headers (part) %100s", response_headers);
	//d2d_dbg("Body (part) %100s", response_body);

	// head and payload separated by \r\n\r\n (one \r\n is at the end of head - maybe)
	if (opts->want_head) {
		// truncate
		if (headers_size > len) {
			response_headers[len] = 0;
			opts->want_body = false; // soz, it wouldn't fit
		}
		sendResponseToUART(response_headers);
		if(opts->want_body) sendResponseToUART("\r\n");
	}

	if(opts->want_body) {
		// truncate
		if (opts->want_head*(headers_size+2)+body_size > len) {
			response_body[len - (opts->want_head*(headers_size+2))] = 0;
		}

		sendResponseToUART(response_body);
	}

	sendResponseToUART("\a");

	free(opts->nonce);
	free(opts);

	request_pending = false;
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
		if (request_pending) return false;

		// Send a esp-esp message
		msg += 2;
		const char *ip;
		FIND_NEXT(ip, ';');
		const char *payload = msg;

		d2d_info("D2D Tx,dest=%s,msg=%s", ip, payload);
		sprintf(buff40, "http://%s" API_D2D_MSG, ip);

		httpclient_args args;
		httpclient_args_init(&args);
		args.method = HTTPD_METHOD_POST;
		args.body = payload;
		args.headers = D2D_HEADERS;
		args.timeout = D2D_TIMEOUT_MS;
		args.url = buff40; // "escapes scope" warning - can ignore, strdup is used

		request_pending = true;
		http_request(&args, requestNoopCb);
		return true;
	}
	else if (strstarts(msg, "H;")) {
		if (request_pending) return false;

		// Send a HTTP request
		msg += 2;
		const char *method = NULL;
		const char *params = NULL;
		const char *nonce = NULL;
		const char *url = NULL;
		const char *payload = NULL;
		httpd_method methodNum;

		FIND_NEXT(method, ';');

		if (streq(method, "GET")) methodNum = HTTPD_METHOD_GET;
		else if (streq(method, "POST")) methodNum = HTTPD_METHOD_POST;
		else if (streq(method, "OPTIONS")) methodNum = HTTPD_METHOD_OPTIONS;
		else if (streq(method, "PUT")) methodNum = HTTPD_METHOD_PUT;
		else if (streq(method, "DELETE")) methodNum = HTTPD_METHOD_DELETE;
		else if (streq(method, "PATCH")) methodNum = HTTPD_METHOD_PATCH;
		else if (streq(method, "HEAD")) methodNum = HTTPD_METHOD_HEAD;
		else {
			d2d_warn("BAD METHOD: %s", method);
			return false;
		}

		FIND_NEXT(params, ';');

		d2d_info("HTTP request");
		d2d_dbg("Method %s", method);
		size_t max_buf_len = HTTPCLIENT_DEF_MAX_LEN;
		size_t max_result_len = 0; // 0 = no truncate
		uint timeout = HTTPCLIENT_DEF_TIMEOUT_MS;
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
			else if(strstarts(param, "l=")) { // max buffer length
				max_buf_len = (size_t) atoi(param + 2);
			} else if(strstarts(param, "L=")) { // max length
				max_result_len = (size_t) atoi(param + 2);
			} else if(strstarts(param, "T=")) { // timeout
				timeout = (uint) atoi(param + 2);
			} else if(strstarts(param, "N=")) { // Nonce
				nonce = param+2;
			} else {
				d2d_warn("BAD PARAM: %s", param);
				return false;
			}

			d2d_dbg("- param %s", params);

			if (p == NULL) break;
			params = p + 1;
		} while(1);

		p = strchr(msg, '\n');
		if (p != NULL) *p = '\0';
		url = msg;
		d2d_dbg("URL: %s", url);

		if (p != NULL)  {
			payload = p + 1;
			d2d_dbg("Payload: %s", payload);
		} else {
			payload = NULL;
		}

		httpclient_args args;
		httpclient_args_init(&args);
		args.method = methodNum;
		args.body = payload;
		args.headers = D2D_HEADERS;
		args.timeout = timeout;
		args.max_response_len = max_buf_len;
		args.url = url;

		if (!no_resp) {
			struct d2d_request_opts *opts = malloc(sizeof(struct d2d_request_opts));
			opts->want_body = want_body;
			opts->want_head = want_head;
			opts->max_result_len = max_result_len;
			opts->nonce = esp_strdup(nonce);
			args.userData = opts;
		}

		request_pending = true;
		http_request(&args, no_resp ? requestNoopCb : requestCb);

		d2d_dbg("Request sent.");
		return true;
	}

	return false;
}

httpd_cgi_state ICACHE_FLASH_ATTR cgiD2DMessage(HttpdConnData *connData)
{
	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	size_t len = 0;
	if (connData->post && connData->post->buff)
		len = strlen(connData->post->buff);
	else if (connData->getArgs)
		len = strlen(connData->getArgs);
	else
		len = 0;

	u8 *ip = connData->remote_ip;
	char buf[20];
	sprintf(buf, "\x1b^m;"IPSTR";L=%d;", ip[0], ip[1], ip[2], ip[3], (int)len);
	sendResponseToUART(buf);

	if (connData->post && connData->post->buff)
		sendResponseToUART(connData->post->buff);
	else if (connData->getArgs)
		sendResponseToUART(connData->getArgs);

	sendResponseToUART("\a");

	d2d_info("D2D Rx src="IPSTR",len=%d", ip[0], ip[1], ip[2], ip[3],len);

	// Received a msg

	httdResponseOptions(connData, 0);
	httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);

	httpdStartResponse(connData, 200);
	httpdHeader(connData, "Content-Type", "text/plain");
	httpdEndHeaders(connData);

	httpdSend(connData, "message received\r\n", -1);

	return HTTPD_CGI_DONE;
}
