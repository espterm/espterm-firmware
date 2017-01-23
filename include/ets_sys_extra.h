#pragma once
#include <c_types.h>

// copied from nodemcu source
extern uint32_t system_get_time();
extern uint32_t platform_tmr_exists(uint32_t t);
extern uint32_t system_rtc_clock_cali_proc();
extern uint32_t system_get_rtc_time();
extern void system_restart();
extern void system_soft_wdt_feed();

// https://github.com/mziwisky/esp8266-dev/blob/master/esphttpd/include/espmissingincludes.h
extern int ets_str2macaddr(void *, void *);
extern void ets_update_cpu_frequency(int freqmhz);
extern int os_printf_plus(const char *format, ...)  __attribute__ ((format (printf, 1, 2)));
//extern uint8 wifi_get_opmode(void);
extern void ets_bzero(void *s, size_t n);
