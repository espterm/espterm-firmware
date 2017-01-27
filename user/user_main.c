/**
 * This is the ESP8266 Remote Terminal project main file.
 *
 * Front-end URLs are defined in routes.c, handlers in cgi_*.c
 */

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include <esp8266.h>
#include <httpdespfs.h>
#include <captdns.h>
#include <espfs.h>
#include <webpages-espfs.h>
#include "serial.h"
#include "io.h"
#include "screen.h"
#include "routes.h"

#define FIRMWARE_VERSION "0.2"

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

static ETSTimer prHeapTimer;

/** Periodically show heap usage */
static void ICACHE_FLASH_ATTR prHeapTimerCb(void *arg)
{
	static uint32_t last = 0;

	uint32_t heap = system_get_free_heap_size();
	int32_t diff = (heap-last);
	const char *cc = "+";
	if (diff<0) cc = "";

	if (diff == 0) {
		dbg("Free heap: %d bytes", heap);
	} else {
		dbg("Free heap: %d bytes (%s%d)", heap, cc, diff);
	}

	last = heap;
}

//Main routine. Initialize stdout, the I/O, filesystem and the webserver and we're done.
void ICACHE_FLASH_ATTR user_init(void)
{
	serialInit();

	printf("\r\n");
	banner("====== ESP8266 Remote Terminal ======");
	banner_info("Firmware (c) Ondrej Hruska, 2017");
	banner_info("github.com/MightyPork/esp-vt100-firmware");
	banner_info("");
	banner_info("Version " FIRMWARE_VERSION ", built " __DATE__ " at " __TIME__);
	banner_info("");
	banner_info("Department of Measurement, CTU Prague");
	printf("\r\n");

	ioInit();

	// 0x40200000 is the base address for spi flash memory mapping, ESPFS_POS is the position
	// where image is written in flash that is defined in Makefile.
#ifdef ESPFS_POS
	espFsInit((void*)(0x40200000 + ESPFS_POS));
#else
	espFsInit((void *) (webpages_espfs_start));
#endif

	// Captive portal
	captdnsInit();

	// Server
	httpdInit(routes, 80);

	// Heap use timer & blink
	os_timer_disarm(&prHeapTimer);
	os_timer_setfn(&prHeapTimer, prHeapTimerCb, NULL);
	os_timer_arm(&prHeapTimer, 5000, 1);

	// The terminal screen
	screen_init();

	info("Listening on UART0, 115200-8-N-1!");
}

// ---- unused funcs removed from sdk to save space ---

// вызывается из phy_chip_v6.o
void ICACHE_FLASH_ATTR chip_v6_set_sense(void)
{
	// ret.n
}

// вызывается из phy_chip_v6.o
int ICACHE_FLASH_ATTR chip_v6_unset_chanfreq(void)
{
	return 0;
}
