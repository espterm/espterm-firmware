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
	bool changed = false;
	if (sysconf->config_version < 1) {
		dbg("Upgrading syscfg to v 1");
		sysconf->overclock = false;
		changed = true;
	}

	sysconf->config_version = SYSCONF_VERSION;

	if (changed) {
		persist_store();
	}

	// uart settings live here, but the CGI handler + form has been moved to the Terminal config page
	serialInit();

	system_update_cpu_freq((uint8) (sysconf->overclock ? 160 : 80));
}

void ICACHE_FLASH_ATTR
sysconf_restore_defaults(void)
{
	sysconf->uart_parity = PARITY_NONE;
	sysconf->uart_baudrate = BIT_RATE_115200;
	sysconf->uart_stopbits = ONE_STOP_BIT;
	sysconf->config_version = SYSCONF_VERSION;
	sysconf->access_pw[0] = 0;
	sysconf->pwlock = PWLOCK_NONE;
	strcpy(sysconf->access_pw, DEF_ACCESS_PW);
	strcpy(sysconf->access_name, DEF_ACCESS_NAME);
	sysconf->overclock = false;
}
