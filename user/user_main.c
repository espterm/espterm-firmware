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
#include <cgiflash.h>
#include "serial.h"
#include "io.h"
#include "screen.h"
#include "routes.h"
#include "version.h"
#include "uart_driver.h"
#include "ansi_parser_callbacks.h"
#include "wifimgr.h"
#include "persist.h"
#include "ansi_parser.h"
#include "ascii.h"
#include "uart_buffer.h"

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

#define HEAP_TIMER_MS 1000
/** Periodically show heap usage */
static void ICACHE_FLASH_ATTR prHeapTimerCb(void *arg)
{
	static int last = 0;
	static int cnt = 0;

	int heap = system_get_free_heap_size();
	int diff = (heap-last);

	int rxc = UART_AsyncRxCount();
	int txc = UART_AsyncTxCount();

	int rxp = ((rxc*10000) / UART_RX_BUFFER_SIZE)/100;
	int txp = ((txc*10000) / UART_TX_BUFFER_SIZE)/100;

	const char *cc = "+";
	if (diff<0) cc = "";

	if (diff == 0) {
		if (cnt == 5) {
			// only every 5 secs if no change
			dbg("Rx: %2d%c, Tx: %2d%c, Hp: %d", rxp, '%', txp, '%', heap);
			cnt = 0;
		}
	} else {
		// report change
		dbg("Rx: %2d%c, Tx: %2d%c, Hp: %d (%s%d)", rxp, '%', txp, '%', heap, cc, diff);
		cnt = 0;
	}

	last = heap;
	cnt++;
}

// Deferred init
static void user_start(void *unused);

static ETSTimer userStartTimer;
static ETSTimer prHeapTimer;

//Main routine. Initialize stdout, the I/O, filesystem and the webserver and we're done.
void ICACHE_FLASH_ATTR user_init(void)
{
	ansi_parser_inhibit = true;
	serialInitBase();

	// Prevent WiFi starting and connecting by default
	// let wifi manager handle it
	wifi_station_set_auto_connect(false);
	wifi_set_opmode(NULL_MODE); // saves to flash if changed - this might avoid the current spike on startup?

	u8 mac[6];
	wifi_get_macaddr(SOFTAP_IF, mac);

	banner_gap();
	banner("================ ESPTerm ================");
	banner_info();
	banner_info("Project by Ondrej Hruska, 2017");
	banner_info();
	banner_info(TERMINAL_GITHUB_REPO_NOPROTO);
	banner_info();
	banner_info("Version "FW_VERSION" ("ESP_LANG"), code name "FW_CODENAME_QUOTED);
	banner_info(" back-end #"GIT_HASH_BACKEND" front-end #"GIT_HASH_FRONTEND);
	banner_info(" built "__DATE__" at "__TIME__" "__TIMEZONE__);
	banner_info();
	banner_info("Device ID: %02X%02X%02X", mac[3], mac[4], mac[5]);
	banner_gap();

	ioInit();

	// 0x40200000 is the base address for spi flash memory mapping, ESPFS_POS is the position
	// where image is written in flash that is defined in Makefile.
#ifdef ESPFS_POS
	espFsInit((void*)(0x40200000 + ESPFS_POS));
#else
	espFsInit((void *) (webpages_espfs_start));
#endif

#if DEBUG_HEAP
	// Heap use timer & blink
	TIMER_START(&prHeapTimer, prHeapTimerCb, HEAP_TIMER_MS, 1);
#endif

	// do later (some functions do not work if called from user_init)
	TIMER_START(&userStartTimer, user_start, 10, 0);
}

static void ICACHE_FLASH_ATTR user_start(void *unused)
{
	// Load and apply stored settings, or defaults if stored settings are invalid
	persist_load();

	captdnsInit();
	httpdInit(routes, 80);
	httpdSetName("ESPTerm " VERSION_STRING);

	ansi_parser_inhibit = false;

	// Print the CANCEL character to indicate the module has restarted
	// Critically important for client application if any kind of screen persistence / content re-use is needed
	UART_WriteChar(UART0, CAN, UART_TIMEOUT_US); // 0x18 - 24 - CAN
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
