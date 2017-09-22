//
// Created by MightyPork on 2017/07/09.
//
// There are 4 sets of settings.
// - hard defaults - hardcoded in firmware, used for init defaults after flash or if stored data are corrupt
// - defaults - persisted by privileged user
// - current - persistent current config state, can be restored to defaults any time
// - live - non-persistent settings valid only for the current runtime

#ifndef ESP_VT100_FIRMWARE_PERSIST_H
#define ESP_VT100_FIRMWARE_PERSIST_H

#include "wifimgr.h"
#include "screen.h"
#include "syscfg.h"

#define DEFAULT_ADMIN_PW "adminpw"

// Changing this could be used to force-erase the config area
// after a firmware upgrade
#define CHECKSUM_SALT 5

#define APPCONF_SIZE 1900

/** Struct for current or default settings */
typedef struct { // the entire block should be 1024 bytes long (for compatibility across upgrades)
	WiFiConfigBundle wificonf;
	uint8_t _filler1[WIFICONF_SIZE - sizeof(WiFiConfigBundle)];

	SystemConfigBundle sysconf;
	uint8_t _filler3[SYSCONF_SIZE - sizeof(SystemConfigBundle)];

	TerminalConfigBundle termconf;
	uint8_t _filler2[TERMCONF_SIZE - sizeof(TerminalConfigBundle)];

	// --- Space for future settings ---
	// The size must be appropriately reduced each time something is added,
	// and boolean flags defaulting to 0 should be used to detect unpopulated
	// sections that must be restored to defaults on load.
	//
	// This ensures user settings are not lost each time they upgrade the firmware,
	// which would lead to a checksum mismatch if the structure was changed and
	// it grew to a different memory area.
	uint8_t _filler_end[
		APPCONF_SIZE
		- 4 // checksum
		- WIFICONF_SIZE
		- SYSCONF_SIZE
		- TERMCONF_SIZE
	];

	uint32_t checksum; // computed before write and tested on load. If it doesn't match, values are reset to hard defaults.
} AppConfigBundle;

#define ADMINCONF_VERSION 1
#define ADMINCONF_SIZE 256

typedef struct {
	u8 version;
	char pw[64];
	uint8_t _filler[ADMINCONF_SIZE-64-4];
	uint32_t checksum;
} AdminConfigBlock;

/** This is the entire data block stored in FLASH */
typedef struct {
	AppConfigBundle defaults; // defaults are stored here
	AppConfigBundle current;     // active settings adjusted by the user
	AdminConfigBlock admin;
} PersistBlock;

// Persist holds the data currently loaded from the flash
extern PersistBlock persist;

void persist_load(void);
void persist_load_hard_default(void);
void persist_restore_default(void);
void persist_set_as_default(void);
void persist_store(void);

#if DEBUG_PERSIST
#define persist_warn warn
#define persist_dbg dbg
#define persist_info info
#else
#define persist_warn(fmt, ...)
#define persist_dbg(fmt, ...)
#define persist_info(fmt, ...)
#endif

#endif //ESP_VT100_FIRMWARE_PERSIST_H
