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

// Changing this could be used to force-erase the config area
// after a firmware upgrade
#define CHECKSUM_SALT 0x5F5F5F5F

/** Struct for current or default settings */
typedef struct {
	WiFiConfigBundle wificonf;
	TerminalConfigBundle termconf;

	// --- Space for future settings ---
	// Original size: 1024
	//
	// The size must be appropriately reduced each time something is added,
	// and boolean flags defaulting to 0 should be used to detect unpopulated
	// sections that must be restored to defaults on load.
	//
	// This ensures user settings are not lost each time they upgrade the firmware,
	// which would lead to a checksum mismatch if the structure was changed and
	// it grew to a different memory area.
	uint8_t filler[1024];

	uint32_t checksum; // computed before write and tested on load. If it doesn't match, values are reset to hard defaults.
} AppConfigBundle;

/** This is the entire data block stored in FLASH */
typedef struct {
	AppConfigBundle defaults; // defaults are stored here
	AppConfigBundle current;     // active settings adjusted by the user
} PersistBlock;

// Persist holds the data currently loaded from the flash
extern PersistBlock persist;

void persist_load(void);
void persist_restore_hard_default(void);
void persist_restore_default(void);
void persist_set_as_default(void);
void persist_store(void);

#endif //ESP_VT100_FIRMWARE_PERSIST_H
