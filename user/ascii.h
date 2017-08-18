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
	BEL = 7,
	BS = 8,
	TAB = 9,
	LF = 10,
	VT = 11,
	FF = 12,
	CR = 13,
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
	ESC = 27,
	FS = 28,
	GS = 29,
	RS = 30,
	US = 31,
	SP = 32,
	DEL = 127,
};

#endif //ESP_VT100_FIRMWARE_ASCII_H
