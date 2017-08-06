//
// Created by MightyPork on 2017/07/29.
//

#ifndef ESP_VT100_FIRMWARE_SYSCFG_H
#define ESP_VT100_FIRMWARE_SYSCFG_H

#include <esp8266.h>

// Size designed for the wifi config structure
// Must be constant to avoid corrupting user config after upgrade
#define SYSCONF_SIZE 200

typedef struct {
	u32 uart_baudrate;
	u8 uart_parity;
	u8 uart_stopbits;
} SystemConfigBundle;

extern SystemConfigBundle * const sysconf;

void sysconf_apply_settings(void);

void sysconf_restore_defaults(void);

#endif //ESP_VT100_FIRMWARE_SYSCFG_H
