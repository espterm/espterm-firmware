//
// Created by MightyPork on 2017/09/06.
//

#ifndef ESPTERM_CHARACTER_SETS_H_H
#define ESPTERM_CHARACTER_SETS_H_H

#include <c_types.h>

// Tables must be contiguous!

#define CODEPAGE_A_BEGIN 35
#define CODEPAGE_A_END 35

static const u16 codepage_A[] ESP_CONST_DATA =
	{//	Unicode    ASCII   SYM
		// %%BEGIN:A%%
		0x20a4, // 35   #   £
		// %%END:A%%
	};

#define CODEPAGE_0_BEGIN 96
#define CODEPAGE_0_END 126

/**
 * translates VT100 ACS escape codes to Unicode values.
 * Based on rxvt-unicode screen.C table.
 */
static const u16 codepage_0[] ESP_CONST_DATA =
	{//	Unicode    ASCII   SYM
		// %%BEGIN:0%%
		0x2666, // 96	`	♦
		0x2592, // 97	a	▒
		0x2409, // 98	b	HT
		0x240c, // 99	c	FF
		0x240d, // 100	d	CR
		0x240a, // 101	e	LF
		0x00b0, // 102	f	°
		0x00b1, // 103	g	±
		0x2424, // 104	h	NL
		0x240b, // 105	i	VT
		0x2518, // 106	j	┘
		0x2510, // 107	k	┐
		0x250c, // 108	l	┌
		0x2514, // 109	m	└
		0x253c, // 110	n	┼
		0x23ba, // 111	o	⎺
		0x23bb, // 112	p	⎻
		0x2500, // 113	q	─
		0x23bc, // 114	r	⎼
		0x23bd, // 115	s	⎽
		0x251c, // 116	t	├
		0x2524, // 117	u	┤
		0x2534, // 118	v	┴
		0x252c, // 119	w	┬
		0x2502, // 120	x	│
		0x2264, // 121	y	≤
		0x2265, // 122	z	≥
		0x03c0, // 123	{	π
		0x2260, // 124	|	≠
		0x20a4, // 125	}	£
		0x00b7, // 126	~	·
		// %%END:0%%
	};

#define CODEPAGE_1_BEGIN 33
#define CODEPAGE_1_END 126

static const u16 codepage_1[] ESP_CONST_DATA =
	{//	Unicode    ASCII   SYM  DOS
		// %%BEGIN:1%%
		0x263A,	// 33	!	☺	(1)  - low ASCII symbols from DOS, moved to +32
		0x263B,	// 34	"	☻	(2)
		0x2665,	// 35	#	♥	(3)
		0x2666,	// 36	$	♦	(4)
		0x2663,	// 37	%	♣	(5)
		0x2660,	// 38	&	♠	(6)
		0x2022,	// 39	'	•	(7)  - inverse dot and circle left out, can be done with SGR
		0x231B,	// 40	(	⌛        - hourglass (timer icon)
		0x25CB,	// 41	)	○	(9)
		0x21AF,	// 42	*	↯        - electricity (lightning monitor...)
		0x266A,	// 43	+	♪	(13)
		0x266B,	// 44	,	♫	(14)
		0x263C,	// 45	-	☼	(15)
		0x2302,	// 46	.	⌂	(127)
		0x2622,	// 47	/	☢         - radioactivity (geiger counter...)
		0x2591,	// 48	0	░	(176) - this block is kept aligned and ordered from DOS, moved -128
		0x2592,	// 49	1	▒	(177)
		0x2593,	// 50	2	▓	(178)
		0x2502,	// 51	3	│	(179)
		0x2524,	// 52	4	┤	(180)
		0x2561,	// 53	5	╡	(181)
		0x2562,	// 54	6	╢	(182)
		0x2556,	// 55	7	╖	(183)
		0x2555,	// 56	8	╕	(184)
		0x2563,	// 57	9	╣	(185)
		0x2551,	// 58	:	║	(186)
		0x2557,	// 59	;	╗	(187)
		0x255D,	// 60	<	╝	(188)
		0x255C,	// 61	=	╜	(189)
		0x255B,	// 62	>	╛	(190)
		0x2510,	// 63	?	┐	(191)
		0x2514,	// 64	@	└	(192)
		0x2534,	// 65	A	┴	(193)
		0x252C,	// 66	B	┬	(194)
		0x251C,	// 67	C	├	(195)
		0x2500,	// 68	D	─	(196)
		0x253C,	// 69	E	┼	(197)
		0x255E,	// 70	F	╞	(198)
		0x255F,	// 71	G	╟	(199)
		0x255A,	// 72	H	╚	(200)
		0x2554,	// 73	I	╔	(201)
		0x2569,	// 74	J	╩	(202)
		0x2566,	// 75	K	╦	(203)
		0x2560,	// 76	L	╠	(204)
		0x2550,	// 77	M	═	(205)
		0x256C,	// 78	N	╬	(206)
		0x2567,	// 79	O	╧	(207)
		0x2568,	// 80	P	╨	(208)
		0x2564,	// 81	Q	╤	(209)
		0x2565,	// 82	R	╥	(210)
		0x2559,	// 83	S	╙	(211)
		0x2558,	// 84	T	╘	(212)
		0x2552,	// 85	U	╒	(213)
		0x2553,	// 86	V	╓	(214)
		0x256B,	// 87	W	╫	(215)
		0x256A,	// 88	X	╪	(216)
		0x2518,	// 89	Y	┘	(217)
		0x250C,	// 90	Z	┌	(218)
		0x2588,	// 91	[	█	(219)
		0x2584,	// 92	\	▄	(220)
		0x258C,	// 93	]	▌	(221)
		0x2590,	// 94	^	▐	(222)
		0x2580,	// 95	_	▀	(223)
		0x2195,	// 96	`	↕	(18)  - moved from low DOS ASCII
		0x2191,	// 97	a	↑	(24)
		0x2193,	// 98	b	↓	(25)
		0x2192,	// 99	c	→	(26)
		0x2190,	// 100	d	←	(27)
		0x2194,	// 101	e	↔	(29)
		0x25B2,	// 102	f	▲	(30)
		0x25BC,	// 103	g	▼	(31)
		0x25BA,	// 104	h	►	(16)
		0x25C4,	// 105	i	◄	(17)
		0x25E2,	// 106	j	◢         - added for slanted corners
		0x25E3,	// 107	k	◣
		0x25E4,	// 108	l	◤
		0x25E5,	// 109	m	◥
		0x256D,	// 110	n	╭         - rounded corners
		0x256E,	// 111	o	╮
		0x256F,	// 112	p	╯
		0x2570,	// 113	q	╰
		0x0,	// 114	r             - free positions for future expansion
		0x0,	// 115	s
		0x0,	// 116	t
		0x0,	// 117	u
		0x0,	// 118	v
		0x0,	// 119	w
		0x0,	// 120	x
		0x0,	// 121	y
		0x0,	// 122	z
		0x0,	// 123	{
		0x0,	// 124	|
		0x2714,	// 125	}	✔         - checkboxes or checklist items
		0x2718,	// 126	~	✘
		// %%END:1%%
	};

#endif //ESPTERM_CHARACTER_SETS_H_H
