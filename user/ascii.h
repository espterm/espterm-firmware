//
// Created by MightyPork on 2017/08/19.
//

#ifndef ESP_VT100_FIRMWARE_ASCII_H
#define ESP_VT100_FIRMWARE_ASCII_H

enum ASCII_CODES {
	NUL = 0,
	SOH = 1,
	STX = 2,
	ETX = 3,
	EOT = 4,
	ENQ = 5,
	ACK = 6,
	BEL = 7, /* \a */
	BS = 8,  /* \b */
	TAB = 9, /* \t */
	LF = 10, /* \n */
	VT = 11, /* \v */
	FF = 12, /* \f */
	CR = 13, /* \r */
	SO = 14,
	SI = 15,
	DLE = 16,
	DC1 = 17,
	DC2 = 18,
	DC3 = 19,
	DC4 = 20,
	NAK = 21,
	SYN = 22,
	ETB = 23,
	CAN = 24,
	EM = 25,
	SUB = 26,
	ESC = 27, /* \033, \x1b, \e */
	FS = 28,
	GS = 29,
	RS = 30,
	US = 31,
	SP = 32,
	DEL = 127,
};

#endif //ESP_VT100_FIRMWARE_ASCII_H
