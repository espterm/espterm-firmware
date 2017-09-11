//
// Created by MightyPork on 2017/09/10.
//

#include <esp8266.h>
#include "utf8.h"

typedef struct {
	char bytes[4];
	uint16_t count;
} UnicodeCacheSlot;

static UnicodeCacheSlot cache[UNICODE_CACHE_SIZE];

#define REF_TO_ID(c) (u8)(c > 127 ? c & 0x7f + 32 : c)
#define ID_TO_REF(c) (UnicodeCacheRef)(c > 31 ? c + 95 : 95)
#define IS_UNICODE_CACHE_REF(c) (c < 32 || c >= 127)

/**
 * Add a code point to the cache. ASCII is passed through.
 * If the code point is already stored, its use counter is incremented.
 *
 * @param bytes - utf8 bytes
 * @return the obtained look-up reference
 */
UnicodeCacheRef ICACHE_FLASH_ATTR
unicode_cache_add(const u8 *bytes) {
	if (bytes[0] < 32) {
		utfc_warn("utf8 cache bad char '%c'", bytes[0]);
		return '?';
	}
	if (bytes[0] < 127) return bytes[0]; // ASCII, bypass

	u8 slot;
	for (slot = 0; slot < UNICODE_CACHE_SIZE; slot++) {
		if (strneq(cache[slot].bytes, bytes, 4)) {
			cache[slot].count++;
			if (cache[slot].count == 1) {
				utfc_dbg("utf8 cache resurrect '%.4s' @ %d", bytes, slot);
			} else {
				utfc_dbg("utf8 cache inc '%.4s' @ %d, %d uses", bytes, slot, cache[slot].count);
			}
			goto suc;
		}
	}
	for (slot = 0; slot < UNICODE_CACHE_SIZE; slot++) {
		if (cache[slot].count==0) {
			// empty slot, store it
			strncpy(cache[slot].bytes, bytes, 4); // this will zero out the remainder
			cache[slot].count = 1;
			utfc_dbg("utf8 cache new '%.4s' @ %d", bytes, slot);
			goto suc;
		}
	}
	error("utf8 cache full");
	return '?'; // fallback to normal ASCII that will show to the user
	suc:
	return ID_TO_REF(slot);
}

/**
 * Look up a code point in the cache by reference. Do not change the use counter.
 *
 * @param ref - reference obtained earlier using unicode_cache_add()
 * @param target - buffer of size 4 to hold the result.
 * @return true if the look-up succeeded
 */
bool ICACHE_FLASH_ATTR
unicode_cache_retrieve(UnicodeCacheRef ref, u8 *target) {
	if (!IS_UNICODE_CACHE_REF(ref)) {
		// ASCII, bypass
		target[0] = ref;
		target[1] = 0;
		return true;
	}

	u8 slot = REF_TO_ID(ref);

	if (cache[slot].count == 0) {
		// "use after free"
		target[0] = '?';
		target[1] = 0;
		utfc_warn("utf8 cache use-after-free @ %d (freed)", slot);
		return false;
	}

	utfc_dbg("utf8 cache hit '%.4s' @ %d, uses %d", cache[slot].bytes, slot, cache[slot].count);
	strncpy((char*)target, cache[slot].bytes, 4);
	return true;
}

/**
 * Remove an occurence of a code point from the cache.
 * If the code point is used more than once, the use counter is decremented.
 *
 * @param ref - reference to remove or reduce
 * @return true if the code point was found in the cache
 */
bool ICACHE_FLASH_ATTR
unicode_cache_remove(UnicodeCacheRef ref) {
	if (!IS_UNICODE_CACHE_REF(ref)) return true; // ASCII, bypass

	u8 slot = REF_TO_ID(ref);

	if (cache[slot].count == 0) {
		utfc_warn("utf8 cache double-free @ %d", slot, cache[slot].count);
		return false;
	}

	cache[slot].count--;
	if (cache[slot].count) {
		utfc_dbg("utf8 cache sub '%.4s' @ %d, %d uses remain", cache[slot].bytes, slot, cache[slot].count);
	} else {
		utfc_dbg("utf8 cache del '%.4s' @ %d", cache[slot].bytes, slot, cache[slot].count);
	}
	return true;
}


/**
 * Encode a code point using UTF-8
 *
 * @param out - output buffer (min 4 characters), will be 0-terminated if shorten than 4
 * @param utf - code point 0-0x10FFFF
 * @return number of bytes on success, 0 on failure (also produces U+FFFD, which uses 3 bytes)
 */
int ICACHE_FLASH_ATTR
utf8_encode(char *out, uint32_t utf)
{
	if (utf <= 0x7F) {
		// Plain ASCII
		out[0] = (char) utf;
		out[1] = 0;
		return 1;
	}
	else if (utf <= 0x07FF) {
		// 2-byte unicode
		out[0] = (char) (((utf >> 6) & 0x1F) | 0xC0);
		out[1] = (char) (((utf >> 0) & 0x3F) | 0x80);
		out[2] = 0;
		return 2;
	}
	else if (utf <= 0xFFFF) {
		// 3-byte unicode
		out[0] = (char) (((utf >> 12) & 0x0F) | 0xE0);
		out[1] = (char) (((utf >>  6) & 0x3F) | 0x80);
		out[2] = (char) (((utf >>  0) & 0x3F) | 0x80);
		out[3] = 0;
		return 3;
	}
	else if (utf <= 0x10FFFF) {
		// 4-byte unicode
		out[0] = (char) (((utf >> 18) & 0x07) | 0xF0);
		out[1] = (char) (((utf >> 12) & 0x3F) | 0x80);
		out[2] = (char) (((utf >>  6) & 0x3F) | 0x80);
		out[3] = (char) (((utf >>  0) & 0x3F) | 0x80);
//		out[4] = 0;
		return 4;
	}
	else {
		// error - use replacement character
		out[0] = (char) 0xEF;
		out[1] = (char) 0xBF;
		out[2] = (char) 0xBD;
		out[3] = 0;
		return 0;
	}
}
