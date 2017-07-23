//
// Created by MightyPork on 2017/07/23.
//

#ifndef ESP_VT100_FIRMWARE_HELPERS_H
#define ESP_VT100_FIRMWARE_HELPERS_H

// strcpy that adds 0 at the end of the buffer. Returns void.
#define strncpy_safe(dst, src, n) do { strncpy((char *)(dst), (char *)(src), (n)); (dst)[(n)-1]=0; } while (0)

/**
 * Convert IP hex to arguments for printf.
 * Library IP2STR(ip) does not work correctly due to unaligned memory access.
 */
#define GOOD_IP2STR(ip) ((ip)>>0)&0xff, ((ip)>>8)&0xff, ((ip)>>16)&0xff, ((ip)>>24)&0xff

/**
 * Helper that retrieves an arg from `connData->getArgs` and stores it in `buff`. Returns 1 on success
 */
#define GET_ARG(key) (httpdFindArg(connData->getArgs, key, buff, sizeof(buff)) > 0)

#endif //ESP_VT100_FIRMWARE_HELPERS_H
