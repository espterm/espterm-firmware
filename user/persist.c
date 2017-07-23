//
// Created by MightyPork on 2017/07/09.
//

#include "persist.h"
#include <esp8266.h>
#include "wifimgr.h"
#include "screen.h"

PersistBlock persist;

#define PERSIST_SECTOR_ID 0x3D

//region Persist and restore individual modules

static void ICACHE_FLASH_ATTR
apply_live_settings(void)
{
	dbg("[Persist] Applying live settings...");
	terminal_apply_settings();
	wifimgr_apply_settings();
	// ...
}

static void ICACHE_FLASH_ATTR
restore_live_settings_to_hard_defaults(void)
{
	wifimgr_restore_defaults();
	terminal_restore_defaults();
	// ...
}

//endregion

/**
 * Compute CRC32. Adapted from https://github.com/esp8266/Arduino
 * @param data
 * @param length
 * @return crc32
 */
static uint32_t ICACHE_FLASH_ATTR
calculateCRC32(const uint8_t *data, size_t length)
{
	uint32_t crc = 0xffffffff;
	while (length--) {
		uint8_t c = *data++;
		for (uint32_t i = 0x80; i > 0; i >>= 1) {
			bool bit = (bool) (crc & 0x80000000UL);
			if (c & i) {
				bit = !bit;
			}
			crc <<= 1;
			if (bit) {
				crc ^= 0x04c11db7UL;
			}
		}
	}
	return crc;
}

/**
 * Compute a persist bundle checksum
 *
 * @param bundle
 * @return
 */
static uint32_t ICACHE_FLASH_ATTR
compute_checksum(AppConfigBundle *bundle)
{
	return calculateCRC32((uint8_t *) bundle, sizeof(AppConfigBundle) - 4) ^ CHECKSUM_SALT;
}

/**
 * Load, verify and apply persistent config
 */
void ICACHE_FLASH_ATTR
persist_load(void)
{
	info("[Persist] Loading stored settings from FLASH...");

	dbg("sizeof(AppConfigBundle)      = %d bytes", sizeof(AppConfigBundle));
	dbg("sizeof(PersistBlock)         = %d bytes", sizeof(PersistBlock));
	dbg("sizeof(WiFiConfigBundle)     = %d bytes", sizeof(WiFiConfigBundle));
	dbg("sizeof(TerminalConfigBundle) = %d bytes", sizeof(TerminalConfigBundle));

	bool hard_reset = false;

	// Try to load
	hard_reset |= !system_param_load(PERSIST_SECTOR_ID, 0, &persist, sizeof(PersistBlock));

	// Verify checksums
	if (hard_reset ||
		(compute_checksum(&persist.defaults) != persist.defaults.checksum) ||
		(compute_checksum(&persist.current) != persist.current.checksum)) {
		error("[Persist] Checksum verification: FAILED");
		hard_reset = true;
	} else {
		info("[Persist] Checksum verification: PASSED");
	}

	if (hard_reset) {
		persist_restore_hard_default();
		// this also stores them to flash and applies to modules
	} else {
		apply_live_settings();
	}

	info("[Persist] All settings loaded and applied.");
}

void ICACHE_FLASH_ATTR
persist_store(void)
{
	info("[Persist] Storing all settings to FLASH...");

	// Update checksums before write
	persist.current.checksum = compute_checksum(&persist.current);
	persist.defaults.checksum = compute_checksum(&persist.defaults);

	if (!system_param_save_with_protect(PERSIST_SECTOR_ID, &persist, sizeof(PersistBlock))) {
		error("[Persist] Store to flash failed!");
	}
	info("[Persist] All settings persisted.");
}

/**
 * Restore to built-in defaults
 */
void ICACHE_FLASH_ATTR
persist_restore_hard_default(void)
{
	info("[Persist] Restoring all settings to hard defaults...");

	// Set live config to default values
	restore_live_settings_to_hard_defaults();

	// Store current -> default
	memcpy(&persist.defaults, &persist.current, sizeof(AppConfigBundle));
	persist_store();

	info("[Persist] All settings restored to hard defaults.");

	apply_live_settings(); // apply
}

/**
 * Restore default settings & apply
 */
void ICACHE_FLASH_ATTR
persist_restore_default(void)
{
	info("[Persist] Restoring live settings to stored defaults...");
	memcpy(&persist.current, &persist.defaults, sizeof(AppConfigBundle));
	apply_live_settings();
	info("[Persist] Settings restored to stored defaults.");
}

/**
 * Store current settings as defaults & write to flash
 */
void ICACHE_FLASH_ATTR
persist_set_as_default(void)
{
	info("[Persist] Storing live settings as defaults..");

	// current -> defaults
	memcpy(&persist.defaults, &persist.current, sizeof(AppConfigBundle));
	persist_store();

	info("[Persist] Default settings updated.");
}
