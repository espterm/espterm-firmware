//
// Created by MightyPork on 2017/10/22.
//

#ifndef ESPTERM_CONFIG_XMACROS_H
#define ESPTERM_CONFIG_XMACROS_H

#include <esp8266.h>
#include <helpers.h>

typedef unsigned char uchar;

#define XJOIN(a, b) a##b

/**Do nothing xnotify */
#define xnoop()

/**
 * XGET interface
 *
 * @param buff - buffer where the value should be printed
 * @param value - value to render to the buffer
 */

static inline bool xget_dummy(char *buff, u32 value)
{
	sprintf(buff, "unused %d", value);
	return false;
}

void xget_dec(char *buff, u32 value);
void xget_bool(char *buff, bool value);
void xget_ustring(char *buff, const u8 *value);
void xget_string(char *buff, const char *value);
void xget_ip(char *buff, const struct ip_addr *value);
void xget_dhcp(char *buff, const struct dhcps_lease *value);


/**
 * XSET interface
 *
 * @param name - field name (for debug)
 * @param field - pointer to the target field
 * @param buff - field with the value to be set
 * @param arg - arbitrary argument, used to modify behavior
 *
 * @return xset_result
 */

enum xset_result {
	XSET_FAIL = 0,
	XSET_SET = 1,
	XSET_UNCHANGED = 2,
	XSET_NONE = 3,
};

// Dummy for unimplemented setters
static inline enum xset_result xset_dummy(const char *name, void *field, const char *buff, const void *arg)
{
	return XSET_UNCHANGED;
}

enum xset_result xset_ip(const char *name, struct ip_addr *field, const char *buff, const void *arg);
enum xset_result xset_bool(const char *name, bool *field, const char *buff, const void *arg);
enum xset_result xset_u8(const char *name, u8 *field, const char *buff, const void *arg);
enum xset_result xset_u32(const char *name, u32 *field, const char *buff, const void *arg);
enum xset_result xset_u16(const char *name, u16 *field, const char *buff, const void *arg);

// static string arrays are not &'d, so we don't get **
/** @param arg - max string length */
enum xset_result xset_string(const char *name, char *field, const char *buff, const void *arg);
enum xset_result xset_ustring(const char *name, u8 *field, const char *buff, const void *arg);

/**
 * Helper template macro for CGI functions that load GET args to structs using XTABLE
 *
 * If 'name' is found in connData->getArgs, xset() is called.
 * If the result is SET, xnotify() is fired. Else, 'name,' is appended to the redir_url buffer.
 */
#define XSET_CGI_FUNC(type, name, suffix, deref, xget, xset, xsarg, xnotify, allow) \
	if ((allow) && GET_ARG(#name)) { \
		type *_p = (type *) &XSTRUCT->name; \
		enum xset_result res = xset(#name, _p, buff, (const void*) (xsarg)); \
		if (res == XSET_SET) { xnotify; } \
		else if (res == XSET_FAIL) { redir_url += sprintf(redir_url, #name","); } \
	}

#define XGET_CGI_FUNC(type, name, suffix, deref, xget, xset, xsarg, xnotify, allow) \
	if ((allow) && streq(token, #name)) xget(buff, deref XSTRUCT->name);

#define XGET_CGI_FUNC_RETURN(type, name, suffix, deref, xget, xset, xsarg, xnotify, allow) \
	if ((allow) && streq(token, #name)) { xget(buff, deref XSTRUCT->name); return; }

#define XSTRUCT_FIELD(type, name, suffix, deref, xget, xset, xsarg, xnotify, allow) \
	type name suffix;

#define XDUMP_FIELD(type, name, suffix, deref, xget, allow, xset, xsarg, xnotify) \
	{ xget(buff, deref XSTRUCT->name); dbg(#name " = %s", buff); }

#endif //ESPTERM_CONFIG_XMACROS_H
