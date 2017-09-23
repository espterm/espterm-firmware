//
// Created by MightyPork on 2017/07/29.
//

#ifndef ESP_VT100_FIRMWARE_SYSCFG_H
#define ESP_VT100_FIRMWARE_SYSCFG_H

#include <esp8266.h>

// Size designed for the wifi config structure
// Must be constant to avoid corrupting user config after upgrade
#define SYSCONF_SIZE 300
#define SYSCONF_VERSION 2

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

typedef struct {
	u32 uart_baudrate;
	u8 uart_parity;
	u8 uart_stopbits;
	u8 config_version;
	enum pwlock pwlock : 8; // page access lock
	char access_pw[64]; // access password
	char access_name[32]; // access name
} SystemConfigBundle;

extern SystemConfigBundle * const sysconf;

void sysconf_apply_settings(void);

void sysconf_restore_defaults(void);

#endif //ESP_VT100_FIRMWARE_SYSCFG_H
