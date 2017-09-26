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
	persist_dbg("[Persist] Applying live settings...");

	persist_dbg("[Persist] > system");
	sysconf_apply_settings();

	persist_dbg("[Persist] > wifi");
	wifimgr_apply_settings();

	persist_dbg("[Persist] > terminal");
	terminal_apply_settings();

	persist_dbg("[Persist] Live settings applied.");
	// ...
}

static void ICACHE_FLASH_ATTR
restore_live_settings_to_hard_defaults(void)
{
	persist_dbg("[Persist] Restore to hard defaults...");

	persist_dbg("[Persist] > system");
	sysconf_restore_defaults();

	persist_dbg("[Persist] > wifi");
	wifimgr_restore_defaults();

	persist_dbg("[Persist] > terminal");
	terminal_restore_defaults();

	persist_dbg("[Persist] Restored to hard defaults.");
	// ...
}

//endregion

const u32 wconf_at = (u32)&persist.defaults.wificonf - (u32)&persist.defaults;
const u32 tconf_at = (u32)&persist.defaults.termconf - (u32)&persist.defaults;
const u32 sconf_at = (u32)&persist.defaults.sysconf  - (u32)&persist.defaults;
const u32 cksum_at = (u32)&persist.defaults.checksum - (u32)&persist.defaults;

/**
 * Compute CRC32. Adapted from https://github.com/esp8266/Arduino
 * @param data
 * @param length
 * @return crc32
 */
static uint32_t ICACHE_FLASH_ATTR
calculateCRC32(const uint8_t *data, size_t length)
{
	// the salt here should ensure settings are wiped when the structure changes
	// CHECKSUM_SALT can be adjusted manually to force a reset.
	uint32_t crc = 0xffffffff + CHECKSUM_SALT + ((wconf_at << 16) ^ (tconf_at << 10) ^ (sconf_at << 5));
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
	// this should be the bundle without the checksum
	return calculateCRC32((uint8_t *) bundle, sizeof(AppConfigBundle) - 4);
}

static void ICACHE_FLASH_ATTR
set_admin_block_defaults(void)
{
	persist_info("[Persist] Initing admin config block");
	strcpy(persist.admin.pw, DEFAULT_ADMIN_PW);
	persist.admin.version = ADMINCONF_VERSION;
}

/**
 * Load, verify and apply persistent config
 */
void ICACHE_FLASH_ATTR
persist_load(void)
{
	persist_info("[Persist] Loading settings from FLASH...");

	persist_dbg("Persist memory map:");
	persist_dbg("> wifi  at %4d (error %2d)", wconf_at, wconf_at - 0);
	persist_dbg("> sys   at %4d (error %2d)", sconf_at, sconf_at - WIFICONF_SIZE);
	persist_dbg("> term  at %4d (error %2d)", tconf_at, tconf_at - WIFICONF_SIZE - SYSCONF_SIZE);
	persist_dbg("> cksum at %4d (error %2d)", cksum_at, cksum_at - (APPCONF_SIZE - 4));
	persist_dbg("> Total size = %d bytes (error %d)", sizeof(AppConfigBundle), APPCONF_SIZE - sizeof(AppConfigBundle));

	bool hard_reset = false;

	// Try to load
	hard_reset |= !system_param_load(PERSIST_SECTOR_ID, 0, &persist, sizeof(PersistBlock));

	// Verify checksums
	if (hard_reset ||
		(compute_checksum(&persist.defaults) != persist.defaults.checksum) ||
		(compute_checksum(&persist.current) != persist.current.checksum) ||
		(persist.admin.version != 0 && (calculateCRC32((uint8_t *) &persist.admin, sizeof(AdminConfigBlock) - 4) != persist.admin.checksum))) {
		error("[Persist] Checksum verification: FAILED");
		hard_reset = true;
	} else {
		persist_info("[Persist] Checksum verification: PASSED");
	}

	if (hard_reset) {
		// Zero all out
		memset(&persist, 0, sizeof(PersistBlock));

		persist_load_hard_default();

		// write them also as defaults
		memcpy(&persist.defaults, &persist.current, sizeof(AppConfigBundle));

		// reset admin pw
		set_admin_block_defaults();
		persist_store();

		// this also stores them to flash and applies to modules
	} else {
		if (persist.admin.version == 0) {
			set_admin_block_defaults();
			persist_store();
		}

		apply_live_settings();
	}

	persist_info("[Persist] All settings loaded and applied.");
}

void ICACHE_FLASH_ATTR
persist_store(void)
{
	persist_info("[Persist] Storing all settings to FLASH...");

	// Update checksums before write
	persist.current.checksum = compute_checksum(&persist.current);
	persist.defaults.checksum = compute_checksum(&persist.defaults);
	persist.admin.checksum = calculateCRC32((uint8_t *) &persist.admin, sizeof(AdminConfigBlock) - 4);

	if (!system_param_save_with_protect(PERSIST_SECTOR_ID, &persist, sizeof(PersistBlock))) {
		error("[Persist] Store to flash failed!");
	}
	persist_info("[Persist] All settings persisted.");
}

/**
 * Restore to built-in defaults
 */
void ICACHE_FLASH_ATTR
persist_load_hard_default(void)
{
	persist_info("[Persist] Restoring live settings to hard defaults...");

	// Set live config to default values
	restore_live_settings_to_hard_defaults();
	persist_store();

	persist_info("[Persist] Settings restored to hard defaults.");

	apply_live_settings(); // apply
}

/**
 * Restore default settings & apply. also persists.
 */
void ICACHE_FLASH_ATTR
persist_restore_default(void)
{
	persist_info("[Persist] Restoring live settings to stored defaults...");

	memcpy(&persist.current, &persist.defaults, sizeof(AppConfigBundle));
	apply_live_settings();
	persist_store();

	persist_info("[Persist] Settings restored to stored defaults.");
}

/**
 * Store current settings as defaults & write to flash
 */
void ICACHE_FLASH_ATTR
persist_set_as_default(void)
{
	persist_info("[Persist] Storing live settings as defaults..");

	// current -> defaults
	memcpy(&persist.defaults, &persist.current, sizeof(AppConfigBundle));
	persist_store();

	persist_info("[Persist] Default settings updated.");
}
