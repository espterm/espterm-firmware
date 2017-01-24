
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */


#include <esp8266.h>

#define BTNGPIO 0

/** Set to false if GPIO0 stuck low on boot (after flashing) */
static bool enable_ap_button = false;

static ETSTimer resetBtntimer;

static void ICACHE_FLASH_ATTR resetBtnTimerCb(void *arg) {
	static int resetCnt=0;
	if (enable_ap_button && !GPIO_INPUT_GET(BTNGPIO)) {
		resetCnt++;
	} else {
		if (resetCnt>=6) { //3 sec pressed
			wifi_station_disconnect();
			wifi_set_opmode(STATIONAP_MODE); //reset to AP+STA mode
			info("Reset to AP mode from GPIO0, Restarting system...");
			system_restart();
		}
		resetCnt=0;
	}
}

void ICACHE_FLASH_ATTR ioInit() {
	// GPIO1, GPIO2, GPIO3 - UARTs.

	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
	gpio_output_set(0, 0, 0, (1<<BTNGPIO));

	os_timer_disarm(&resetBtntimer);
	os_timer_setfn(&resetBtntimer, resetBtnTimerCb, NULL);
	os_timer_arm(&resetBtntimer, 500, 1);

	// One way to enter AP mode - hold GPIO0 low.
	if (GPIO_INPUT_GET(BTNGPIO) == 0) {
		// starting "in BOOT mode" - do not install the AP reset timer
		warn("GPIO0 stuck low - AP reset button disabled.\n");
	} else {
		enable_ap_button = true;
		dbg("Note: Hold GPIO0 low for reset to AP mode.\n");
	}
}

