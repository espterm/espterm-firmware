#include <esp8266.h>
#include <httpd.h>

#include "cgi_reset.h"

static ETSTimer tmr;

static void ICACHE_FLASH_ATTR tmrCb(void *arg)
{
	system_restart();
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
