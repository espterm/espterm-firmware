
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
#include "wifimgr.h"
#include "persist.h"

#define BTNGPIO 0

/** Set to false if GPIO0 stuck low on boot (after flashing) */
static bool enable_ap_button = false;

static ETSTimer resetBtntimer;
static ETSTimer blinkyTimer;

// Holding BOOT pin triggers AP reset, then Factory Reset.
// Indicate that by blinking the on-board LED.
// -> ESP-01 has at at GPIO1, ESP-01S at GPIO2. We have to use both for compatibility.

static void ICACHE_FLASH_ATTR bootHoldIndicatorTimerCb(void *arg) {
	static bool state = true;

	if (GPIO_INPUT_GET(BTNGPIO)) {
		// if user released, shut up
		state = 1;
	}

	if (state) {
		GPIO_OUTPUT_SET(1, 1);
		GPIO_OUTPUT_SET(2, 1);
	} else {
		GPIO_OUTPUT_SET(1, 0);
		GPIO_OUTPUT_SET(2, 0);
	}

	state = !state;
}

static void ICACHE_FLASH_ATTR resetBtnTimerCb(void *arg) {
	static int resetCnt=0;
	if (enable_ap_button && !GPIO_INPUT_GET(BTNGPIO)) {
		resetCnt++;

		// indicating AP reset
		if (resetCnt == 2) {
			// LED pin as output (Normally UART output)
			PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_GPIO1);
			PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);

			// LED on
			GPIO_OUTPUT_SET(1, 0);
			GPIO_OUTPUT_SET(2, 0);

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
		// Switch LED pins back to UART mode
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_U1TXD_BK);

		if (resetCnt>=10) { //5 secs pressed - FR (timer is at 500 ms)
			info("BOOT-button triggered FACTORY RESET!");
			persist_restore_default();
		}
		else if (resetCnt>=2) { //1 sec pressed
			info("BOOT-button triggered reset to AP mode...");

			wificonf->opmode = STATIONAP_MODE;
			persist_store();
			wifimgr_apply_settings();
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

