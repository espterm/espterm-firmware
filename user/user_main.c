/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

/**
 * This is the ESP8266 Remote Terminal project main file.
 */

#include <esp8266.h>
#include <httpdespfs.h>
#include <cgiwebsocket.h>
#include <captdns.h>
#include <espfs.h>
#include <webpages-espfs.h>
#include "serial.h"
#include "io.h"
#include "screen.h"

#define FIRMWARE_VERSION "0.1"

/**
 * Broadcast screen state to sockets
 */
void screen_notifyChange() {
	// TODO cooldown / buffering to reduce nr of such events
	dbg("Screen notifyChange");

	void *data = NULL;

	const int bufsiz = 1024;
	char buff[bufsiz];
	for (int i = 0; i < 20; i++) {
		httpd_cgi_state cont = screenSerializeToBuffer(buff, bufsiz, &data);
		cgiWebsockBroadcast("/ws/update.cgi", buff, (int)strlen(buff), (cont == HTTPD_CGI_MORE) ? WEBSOCK_FLAG_CONT : WEBSOCK_FLAG_NONE);
		if (cont == HTTPD_CGI_DONE) break;
	}
}

/** Socket connected for updates */
void ICACHE_FLASH_ATTR myWebsocketConnect(Websock *ws) {
	dbg("Socket connected.");
}

/**
 * Main page template substitution
 *
 * @param connData
 * @param token
 * @param arg
 * @return
 */
httpd_cgi_state ICACHE_FLASH_ATTR tplScreen(HttpdConnData *connData, char *token, void **arg) {
	// cleanup
	if (!connData) {
		// Release data object
		screenSerializeToBuffer(NULL, 0, arg);
		return HTTPD_CGI_DONE;
	}

	const int bufsiz = 1024;
	char buff[bufsiz];

	if (streq(token, "screenData")) {
		httpd_cgi_state cont = screenSerializeToBuffer(buff, bufsiz, arg);
		httpdSend(connData, buff, -1);
		return cont;
	}

	return HTTPD_CGI_DONE;
}


#ifdef ESPFS_POS
CgiUploadFlashDef uploadParams={
	.type=CGIFLASH_TYPE_ESPFS,
	.fw1Pos=ESPFS_POS,
	.fw2Pos=0,
	.fwSize=ESPFS_SIZE,
};
#define INCLUDE_FLASH_FNS
#endif
#ifdef OTA_FLASH_SIZE_K
CgiUploadFlashDef uploadParams={
	.type=CGIFLASH_TYPE_FW,
	.fw1Pos=0x1000,
	.fw2Pos=((OTA_FLASH_SIZE_K*1024)/2)+0x1000,
	.fwSize=((OTA_FLASH_SIZE_K*1024)/2)-0x1000,
	.tagName=OTA_TAGNAME
};
#define INCLUDE_FLASH_FNS
#endif

/** Routes */
HttpdBuiltInUrl builtInUrls[]={ //ICACHE_RODATA_ATTR
	// redirect func for the captive portal
	ROUTE_CGI_ARG("*", cgiRedirectApClientToHostname, "esp8266.nonet"),

	ROUTE_WS("/ws/update.cgi", myWebsocketConnect),

	// TODO add funcs for WiFi management (when web UI is added)

//	ROUTE_TPL_FILE("/", tplScreen, "term.tpl"),
	ROUTE_TPL("/term.tpl", tplScreen),
	ROUTE_FILESYSTEM(),
	ROUTE_END(),
};

static ETSTimer prHeapTimer;

/** Blink & show heap usage */
static void ICACHE_FLASH_ATTR prHeapTimerCb(void *arg) {
	static int led = 0;
	static unsigned int cnt = 0;

	if (cnt%3==0) {
		os_printf("Free heap: %ld bytes\n", (unsigned long) system_get_free_heap_size());
	}

	//cgiWebsockBroadcast("/ws/update.cgi", "HELLO", 5, WEBSOCK_FLAG_NONE);

	ioLed(led);
	led = !led;

	cnt++;
}

//Main routine. Initialize stdout, the I/O, filesystem and the webserver and we're done.
void user_init(void) {
	serialInit();

	banner("ESP8266 Remote Terminal");
	banner_info("Version "FIRMWARE_VERSION", built "__DATE__" at "__TIME__);
	dbg("!!! TODO (c) and GitHub link here !!!");

	//stdoutInit();

	captdnsInit();
	ioInit();

	// 0x40200000 is the base address for spi flash memory mapping, ESPFS_POS is the position
	// where image is written in flash that is defined in Makefile.
#ifdef ESPFS_POS
	espFsInit((void*)(0x40200000 + ESPFS_POS));
#else
	espFsInit((void*)(webpages_espfs_start));
#endif

	httpdInit(builtInUrls, 80);

	// Heap use timer & blink
	os_timer_disarm(&prHeapTimer);
	os_timer_setfn(&prHeapTimer, prHeapTimerCb, NULL);
	os_timer_arm(&prHeapTimer, 1000, 1);

	screen_init();

	info("System ready!");
}

// ---- unused funcs removed from sdk to save space ---

void user_rf_pre_init() {
	//Not needed, but some SDK versions want this defined.
}

// вызывается из phy_chip_v6.o
void ICACHE_FLASH_ATTR chip_v6_set_sense(void) {
	// ret.n
}

// вызывается из phy_chip_v6.o
int ICACHE_FLASH_ATTR chip_v6_unset_chanfreq(void) {
	return 0;
}