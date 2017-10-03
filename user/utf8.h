//
// Created by MightyPork on 2017/09/10.
//

#ifndef ESPTERM_UTF8_H
#define ESPTERM_UTF8_H

#include <c_types.h>

// 160 is maximum possible
#define UNICODE_CACHE_SIZE 160

typedef u8 UnicodeCacheRef;
#define IS_UNICODE_CACHE_REF(c) ((c) < 32 || (c) >= 127)

/**
 * Clear the entire cache
 * @return
 */
void unicode_cache_clear(void);

/**
 * Add a code point to the cache. ASCII is passed through.
 * If the code point is already stored, its use counter is incremented.
 *
 * @param bytes - utf8 bytes
 * @return the obtained look-up reference
 */
UnicodeCacheRef unicode_cache_add(const u8 *bytes);

/**
 * Increment a reference
 *
 * @param ref - reference
 * @return success
 */
bool unicode_cache_inc(UnicodeCacheRef ref);

/**
 * Look up a code point in the cache by reference. Do not change the use counter.
 *
 * @param ref - reference obtained earlier using unicode_cache_add()
 * @param target - buffer of size 4 to hold the result.
 * @return true if the look-up succeeded
 */
bool unicode_cache_retrieve(UnicodeCacheRef ref, u8 *target);

/**
 * Remove an occurence of a code point from the cache.
 * If the code point is used more than once, the use counter is decremented.
 *
 * @param ref - reference to remove or reduce
 * @return true if the code point was found in the cache
 */
bool unicode_cache_remove(UnicodeCacheRef ref);


/**
 * Encode a code point using UTF-8
 *
 * @author Ondřej Hruška <ondra@ondrovo.com>
 * @license MIT
 *
 * @param out - output buffer (min 4 characters), will be 0-terminated if shorten than 4
 * @param utf - code point 0-0x10FFFF
 * @param surrogateFix - add 0x800 to 0xD800-0xDFFF to avoid invalid code points
 * @return number of bytes on success, 0 on failure (also produces U+FFFD, which uses 3 bytes)
 */
int utf8_encode(char *out, uint32_t utf, bool surrogateFix);

#if DEBUG_UTFCACHE
#define utfc_warn warn
#define utfc_dbg dbg
#define utfc_info info
#else
#define utfc_warn(fmt, ...)
#define utfc_dbg(fmt, ...)
#define utfc_info(fmt, ...)
#endif

#endif //ESPTERM_UTF8_H
