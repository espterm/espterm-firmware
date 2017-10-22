//
// Created by MightyPork on 2017/10/22.
//

#include "config_xmacros.h"
#include "cgi_logging.h"

void ICACHE_FLASH_ATTR xget_dec(char *buff, u32 value)
{
	sprintf(buff, "%d", value);
}

void ICACHE_FLASH_ATTR xget_bool(char *buff, bool value)
{
	sprintf(buff, "%d", value?1:0);
}

void ICACHE_FLASH_ATTR xget_ustring(char *buff, const u8 *value)
{
	sprintf(buff, "%s", (const char *) value);
}

void ICACHE_FLASH_ATTR xget_string(char *buff, const char *value)
{
	sprintf(buff, "%s", value);
}

void ICACHE_FLASH_ATTR xget_ip(char *buff, const struct ip_addr *value)
{
	sprintf(buff, IPSTR, GOOD_IP2STR(value->addr));
}

// ------------- XSET -------------

enum xset_result ICACHE_FLASH_ATTR
xset_ip(const char *name, struct ip_addr *field, const char *buff, const void *arg)
{
	cgi_dbg("Setting %s = %s", name, buff);
	u32 ip = ipaddr_addr(buff);
	if (ip != 0 && ip != 0xFFFFFFFFUL) {
		if (field->addr != ip) {
			field->addr = ip;
			return XSET_SET;
		}
		return XSET_UNCHANGED;
	} else {
		cgi_warn("Bad IP: %s", buff);
		return XSET_FAIL;
	}
}

enum xset_result ICACHE_FLASH_ATTR
xset_bool(const char *name, bool *field, const char *buff, const void *arg)
{
	cgi_dbg("Setting %s = %s", name, buff);
	bool enable = (atoi(buff) != 0);

	if (*field != enable) {
		*field = enable;
		return XSET_SET;
	}
	return XSET_UNCHANGED;
}

enum xset_result ICACHE_FLASH_ATTR
xset_u8(const char *name, u8 *field, const char *buff, const void *arg)
{
	cgi_dbg("Setting %s = %s", name, buff);
	u32 val = (u32) atoi(buff);

	if (val <= 255) {
		if (*field != val) {
			*field = (u8) val;
			return XSET_SET;
		}
		return XSET_UNCHANGED;
	} else {
		cgi_warn("Bad value, max 255: %s", buff);
		return XSET_FAIL;
	}
}

enum xset_result ICACHE_FLASH_ATTR
xset_string(const char *name, s8 **field, const char *buff, const void *arg)
{
	cgi_dbg("Setting %s = %s", name, buff);
	u32 maxlen = (u32) arg;

	if (arg > 0 && (u32)strlen(buff) > maxlen) {
		cgi_warn("String too long, max %d", maxlen);
		return XSET_FAIL;
	}

	if (!streq(field, buff)) {
		strncpy_safe(field, buff, (u32)arg);
		return XSET_SET;
	}
	return XSET_UNCHANGED;
}


enum xset_result ICACHE_FLASH_ATTR
xset_ustring(const char *name, u8 **field, const char *buff, const void *arg)
{
	cgi_dbg("Setting %s = %s", name, buff);
	u32 maxlen = (u32) arg;

	if (arg > 0 && (u32)strlen(buff) > maxlen) {
		cgi_warn("String too long, max %d", maxlen);
		return XSET_FAIL;
	}

	if (!streq((char *)field, buff)) {
		strncpy_safe(field, buff, (u32)arg);
		return XSET_SET;
	}
	return XSET_UNCHANGED;
}
