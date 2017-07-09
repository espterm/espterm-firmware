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

typedef struct {
	WiFiConfigBundle wificonf;
	TerminalConfigBundle termconf;
	// ...
	// other settings here
	// ...
	uint32_t checksum; // computed before write and tested on load. If it doesn't match, values are reset to hard defaults.
} AppConfigBundle;

typedef struct {
	AppConfigBundle defaults; // defaults are stored here
	AppConfigBundle current;  // settings persisted by user
} PersistBlock;

// Persist holds the data currently loaded from the flash
extern PersistBlock persist;

void persist_load(void);
void persist_restore_hard_default(void);
void persist_restore_default(void);
void persist_set_as_default(void);
void persist_store(void);

#endif //ESP_VT100_FIRMWARE_PERSIST_H
