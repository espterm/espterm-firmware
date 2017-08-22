//
// Created by MightyPork on 2017/08/20.
//
// Logging functions for the parser, can be switched off with a compile flag
//

#ifndef ESP_VT100_FIRMWARE_APARS_LOGGING_H
#define ESP_VT100_FIRMWARE_APARS_LOGGING_H

#include <esp8266.h>

#ifndef DEBUG_ANSI
#define DEBUG_ANSI 0
#endif

#ifndef DEBUG_ANSI_NOIMPL
#define DEBUG_ANSI_NOIMPL 0
#endif

// defined in the makefile
#if DEBUG_ANSI
#define ansi_warn warn
#define ansi_dbg dbg
#else
#define ansi_warn(fmt, ...)
#define ansi_dbg(fmt, ...)
#endif

#if DEBUG_ANSI_NOIMPL
#define ansi_noimpl(fmt, ...) warn("NOIMPL: " fmt, ##__VA_ARGS__)
#define ansi_noimpl_r(fmt, ...) warn(fmt, ##__VA_ARGS__)
#else
#define ansi_noimpl(fmt, ...)
#define ansi_noimpl_r(fmt, ...)
#endif

#endif //ESP_VT100_FIRMWARE_APARS_LOGGING_H
