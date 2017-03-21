
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */


#include <esp8266.h>
#include "ansi_parser_callbacks.h"

#define BTNGPIO 0

/** Set to false if GPIO0 stuck low on boot (after flashing) */
static bool enable_ap_button = false;

static ETSTimer resetBtntimer;
static ETSTimer blinkyTimer;

static void ICACHE_FLASH_ATTR bootHoldIndicatorTimerCb(void *arg) {
	static bool state = true;

	if (GPIO_INPUT_GET(BTNGPIO)) {
		// if user released, shut up
		state = 1;
	}

	if (state) {
		GPIO_OUTPUT_SET(1, 1);
	} else {
		GPIO_OUTPUT_SET(1, 0);
	}

	state = !state;
}

static void ICACHE_FLASH_ATTR resetBtnTimerCb(void *arg) {
	static int resetCnt=0;
	if (enable_ap_button && !GPIO_INPUT_GET(BTNGPIO)) {
		resetCnt++;

		// indicating AP reset
		if (resetCnt == 2) {
			PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_GPIO1);
			GPIO_OUTPUT_SET(1, 0); // GPIO 1 OFF

			os_timer_disarm(&blinkyTimer);
			os_timer_setfn(&blinkyTimer, bootHoldIndicatorTimerCb, NULL);
			os_timer_arm(&blinkyTimer, 500, 1);
		}

		// indicating we'll perform a factory reset
		if (resetCnt == 10) {
			os_timer_disarm(&blinkyTimer);
			os_timer_setfn(&blinkyTimer, bootHoldIndicatorTimerCb, NULL);
			os_timer_arm(&blinkyTimer, 100, 1);
		}
	} else {
		// Switch Tx back to UART pin, so we can print our farewells
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);

		if (resetCnt>=10) { //5 secs pressed - FR
			info("BOOT-button triggered FACTORY RESET!");
			apars_handle_OSC_FactoryReset();
		}
		else if (resetCnt>=2) { //1 sec pressed
			wifi_station_disconnect();
			wifi_set_opmode(STATIONAP_MODE); //reset to AP+STA mode
			info("BOOT-button triggered reset to AP mode, restarting...");

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

