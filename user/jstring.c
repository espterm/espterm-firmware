//
// Created by MightyPork on 2017/09/04.
//

#include "jstring.h"

void ICACHE_FLASH_ATTR
encode2B(u16 number, WordB2 *stru)
{
	stru->lsb = (u8) (number % 127);
	number = (u16) ((number - stru->lsb) / 127);
	stru->lsb += 1;

	stru->msb = (u8) (number + 1);
}

void ICACHE_FLASH_ATTR
encode3B(u32 number, WordB3 *stru)
{
	stru->lsb = (u8) (number % 127);
	number = (number - stru->lsb) / 127;
	stru->lsb += 1;

	stru->msb = (u8) (number % 127);
	number = (number - stru->msb) / 127;
	stru->msb += 1;

	stru->xsb = (u8) (number + 1);
}

u16 ICACHE_FLASH_ATTR
parse2B(const char *str)
{
	return (u16) ((str[0] - 1) + (str[1] - 1) * 127);
}

u32 ICACHE_FLASH_ATTR
parse3B(const char *str)
{
	return (u32) ((str[0] - 1) + (str[1] - 1) * 127 + (str[2] - 1) * 127 * 127);
}
