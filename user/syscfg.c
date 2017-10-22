//
// Created by MightyPork on 2017/07/29.
//

#include "syscfg.h"
#include "persist.h"
#include "uart_driver.h"
#include "serial.h"
#include "cgi_logging.h"

SystemConfigBundle * const sysconf = &persist.current.sysconf;

enum xset_result ICACHE_FLASH_ATTR
xset_sys_baudrate(const char *name, u32 *field, const char *buff, const void *arg)
{
	cgi_dbg("Setting %s = %s", name, buff);
	int baud = atoi(buff);
	if (baud == BIT_RATE_300 ||
		baud == BIT_RATE_600 ||
		baud == BIT_RATE_1200 ||
		baud == BIT_RATE_2400 ||
		baud == BIT_RATE_4800 ||
		baud == BIT_RATE_9600 ||
		baud == BIT_RATE_19200 ||
		baud == BIT_RATE_38400 ||
		baud == BIT_RATE_57600 ||
		baud == BIT_RATE_74880 ||
		baud == BIT_RATE_115200 ||
		baud == BIT_RATE_230400 ||
		baud == BIT_RATE_460800 ||
		baud == BIT_RATE_921600 ||
		baud == BIT_RATE_1843200 ||
		baud == BIT_RATE_3686400) {
		if (*field != baud) {
			*field = (u32) baud;
			return XSET_SET;
		}
		return XSET_UNCHANGED;
	} else {
		cgi_warn("Bad baud rate %s", buff);
		return XSET_FAIL;
	}
}

enum xset_result ICACHE_FLASH_ATTR
xset_sys_parity(const char *name, u8 *field, const char *buff, const void *arg)
{
	cgi_dbg("Setting %s = %s", name, buff);
	int parity = atoi(buff);
	if (parity >= 0 && parity <= 2) {
		if (*field != parity) {
			*field = (UartParityMode) parity;
			return XSET_SET;
		}
		return XSET_UNCHANGED;
	} else {
		cgi_warn("Bad parity %s", buff);
		return XSET_FAIL;
	}
}

enum xset_result ICACHE_FLASH_ATTR
xset_sys_stopbits(const char *name, u8 *field, const char *buff, const void *arg)
{
	cgi_dbg("Setting %s = %s", name, buff);
	int stopbits = atoi(buff);
	if (stopbits >= 1 && stopbits <= 3) {
		if (*field != stopbits) {
			*field = (UartParityMode) stopbits;
			return XSET_SET;
		}
		return XSET_UNCHANGED;
	} else {
		cgi_warn("Bad stopbits %s", buff);
		return XSET_FAIL;
	}
}

enum xset_result ICACHE_FLASH_ATTR
xset_sys_pwlock(const char *name, u8 *field, const char *buff, const void *arg)
{
	cgi_dbg("Setting %s = %s", name, buff);

	int pwlock = atoi(buff);
	if (pwlock >= 0 && pwlock < PWLOCK_MAX) {
		if (*field != pwlock) {
			*field = (enum pwlock) pwlock;
			return XSET_SET;
		}
		return XSET_UNCHANGED;
	} else {
		cgi_warn("Bad pwlock %s", buff);
		return XSET_FAIL;
	}
}

enum xset_result ICACHE_FLASH_ATTR
xset_sys_accesspw(const char *name, u8 **field, const char *buff, const void *arg)
{
	// Do not overwrite pw if empty
	if (strlen(buff) == 0) return XSET_UNCHANGED;
	return xset_ustring(name, field, buff, arg);
}


void ICACHE_FLASH_ATTR
sysconf_apply_settings(void)
{
//	char buff[64];
//#define XSTRUCT sysconf
//#define X XDUMP_FIELD
//	XTABLE_SYSCONF
//#undef X

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
	strcpy((char *)sysconf->access_pw, DEF_ACCESS_PW);
	strcpy((char *)sysconf->access_name, DEF_ACCESS_NAME);
	sysconf->overclock = false;
}
