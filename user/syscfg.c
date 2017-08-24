//
// Created by MightyPork on 2017/07/29.
//

#include "syscfg.h"
#include "persist.h"
#include "uart_driver.h"
#include "serial.h"

SystemConfigBundle * const sysconf = &persist.current.sysconf;

void ICACHE_FLASH_ATTR
sysconf_apply_settings(void)
{
	// !!! Update to current version !!!

	serialInit();
}

void ICACHE_FLASH_ATTR
sysconf_restore_defaults(void)
{
	sysconf->uart_parity = PARITY_NONE;
	sysconf->uart_baudrate = BIT_RATE_115200;
	sysconf->uart_stopbits = ONE_STOP_BIT;
}
