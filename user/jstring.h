//
// Created by MightyPork on 2017/09/04.
//

#ifndef ESPTERM_JSTRING_H
#define ESPTERM_JSTRING_H

#include <esp8266.h>

typedef struct {
	u8 lsb;
	u8 msb;
} WordB2;

typedef struct {
	u8 lsb;
	u8 msb;
	u8 xsb;
} WordB3;

void encode2B(u16 number, WordB2 *stru);

void encode3B(u32 number, WordB3 *stru);

u16 parse2B(const char *str);

u32 parse3B(const char *str);

#endif //ESPTERM_JSTRING_H
