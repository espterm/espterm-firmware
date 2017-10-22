//
// Created by MightyPork on 2017/07/29.
//

#ifndef ESP_VT100_FIRMWARE_SYSCFG_H
#define ESP_VT100_FIRMWARE_SYSCFG_H

#include <esp8266.h>
#include "config_xmacros.h"

// Size designed for the wifi config structure
// Must be constant to avoid corrupting user config after upgrade
#define SYSCONF_SIZE 300
#define SYSCONF_VERSION 1

#define DEF_ACCESS_PW "1234"
#define DEF_ACCESS_NAME "espterm"

enum pwlock {
	PWLOCK_NONE = 0,
	PWLOCK_SETTINGS_NOTERM = 1,
	PWLOCK_SETTINGS_ALL = 2,
	PWLOCK_MENUS = 3,
	PWLOCK_ALL = 4,
	PWLOCK_MAX = 5,
};


//....Type................Name..Suffix...............Deref..XGET........Allow..Cast..XSET.........................NOTIFY
// Deref is used to pass the field to xget. Cast is used to convert the &'d field to what xset wants (needed for static arrays)
#define XTABLE_SYSCONF \
	X(u32,             uart_baudrate,  /**/,          /**/, xget_dec,      1,         /**/,   xset_sys_baudrate, NULL,   xnoop) \
	X(u8,              uart_parity, /**/,             /**/, xget_dec,      1,         /**/,   xset_sys_parity, NULL,     xnoop) \
	X(u8,              uart_stopbits,  /**/,          /**/, xget_dec,      1,         /**/,   xset_sys_stopbits, NULL,   xnoop) \
	\
	X(u8,              config_version, /**/,          /**/, xget_dec,      0,         /**/,   xset_u8, NULL,             xnoop) \
	\
	X(u8,              pwlock, /**/,                  /**/, xget_dec,      admin|tpl, /**/,   xset_sys_pwlock, NULL,     xnoop) \
	X(u8,              access_pw, [64],               /**/, xget_ustring,  admin,     (u8**), xset_sys_accesspw, NULL,   xnoop) \
	X(u8,              access_name, [32],             /**/, xget_ustring,  admin|tpl, (u8**), xset_ustring, NULL,        xnoop) \
	\
	X(bool,            overclock, /**/,               /**/, xget_bool,     1,         /**/,   xset_bool, NULL,           xnoop) \


typedef struct {
#define X XSTRUCT_FIELD
	XTABLE_SYSCONF
#undef X

//	u32 uart_baudrate;
//	u8 uart_parity;
//	u8 uart_stopbits;
//	u8 config_version;
//	enum pwlock pwlock : 8; // page access lock
//	char access_pw[64]; // access password
//	char access_name[32]; // access name
//	bool overclock;
} SystemConfigBundle;

extern SystemConfigBundle * const sysconf;

void sysconf_apply_settings(void);

void sysconf_restore_defaults(void);

enum xset_result xset_sys_baudrate(const char *name, u32 *field, const char *buff, const void *arg);
enum xset_result xset_sys_parity(const char *name, u8 *field, const char *buff, const void *arg);
enum xset_result xset_sys_stopbits(const char *name, u8 *field, const char *buff, const void *arg);
enum xset_result xset_sys_pwlock(const char *name, u8 *field, const char *buff, const void *arg);
enum xset_result xset_sys_accesspw(const char *name, u8 **field, const char *buff, const void *arg);

#endif //ESP_VT100_FIRMWARE_SYSCFG_H
