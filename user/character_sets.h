//
// Created by MightyPork on 2017/09/06.
//

#ifndef ESPTERM_CHARACTER_SETS_H_H
#define ESPTERM_CHARACTER_SETS_H_H

#include <c_types.h>

// Tables must be contiguous!

// Full range of UTF-8 is now supported, if needed, but the table must be changed to uint32_t

#define CODEPAGE_0_BEGIN 96
#define CODEPAGE_0_END 126

/**
 * translates VT100 ACS escape codes to Unicode values.
 * Based on rxvt-unicode screen.C table.
 */
static const u16 codepage_0[] ESP_CONST_DATA =
	{   //         Unicode  ASCII   SYM
		// %%BEGIN:0%%
		u'♦',	// 0x2666	96	`
		u'▒',	// 0x2592	97	a
		u'␉',	// 0x2409	98	b
		u'␌',	// 0x240c	99	c	FF
		u'␍',	// 0x240d	100	d	CR
		u'␊',	// 0x240a	101	e	LF
		u'°',	// 0x00b0	102	f
		u'±',	// 0x00b1	103	g
		u'␤',	// 0x2424	104	h	NL
		u'␋',	// 0x240b	105	i	VT
		u'┘',	// 0x2518	106	j
		u'┐',	// 0x2510	107	k
		u'┌',	// 0x250c	108	l
		u'└',	// 0x2514	109	m
		u'┼',	// 0x253c	110	n
		u'⎺',	// 0x23ba	111	o
		u'⎻',	// 0x23bb	112	p
		u'─',	// 0x2500	113	q
		u'⎼',	// 0x23bc	114	r
		u'⎽',	// 0x23bd	115	s
		u'├',	// 0x251c	116	t
		u'┤',	// 0x2524	117	u
		u'┴',	// 0x2534	118	v
		u'┬',	// 0x252c	119	w
		u'│',	// 0x2502	120	x
		u'≤',	// 0x2264	121	y
		u'≥',	// 0x2265	122	z
		u'π',	// 0x03c0	123	{
		u'≠',	// 0x2260	124	|
		u'£',	// 0x20a4	125	}
		u'·',	// 0x00b7	126	~
		// %%END:0%%
	};

#define CODEPAGE_1_BEGIN 33
#define CODEPAGE_1_END 126

static const u16 codepage_1[] ESP_CONST_DATA =
	{//            Unicode  ASCII  DOS
		// %%BEGIN:1%%
		u'☺',	// 0x263A,	33	!	(1)  - low ASCII symbols from DOS, moved to +32
		u'☻',	// 0x263B,	34	"	(2)
		u'♥',	// 0x2665,	35	#	(3)
		u'♦',	// 0x2666,	36	$	(4)
		u'♣',	// 0x2663,	37	%	(5)
		u'♠',	// 0x2660,	38	&	(6)
		u'•',	// 0x2022,	39	'	(7)  - inverse dot and circle left out, can be done with SGR
		u'⌛',	// 0x231B,	40	(	- hourglass (timer icon)
		u'○',	// 0x25CB,	41	)	(9)
		u'↯',	// 0x21AF,	42	*	- electricity (lightning monitor...)
		u'♪',	// 0x266A,	43	+	(13)
		u'♫',	// 0x266B,	44	,	(14)
		u'☼',	// 0x263C,	45	-	(15)
		u'⌂',	// 0x2302,	46	.	(127)
		u'☢',	// 0x2622,	47	/	- radioactivity (geiger counter...)
		u'░',	// 0x2591,	48	0	(176) - this block is kept aligned and ordered from DOS, moved -128
		u'▒',	// 0x2592,	49	1	(177)
		u'▓',	// 0x2593,	50	2	(178)
		u'│',	// 0x2502,	51	3	(179)
		u'┤',	// 0x2524,	52	4	(180)
		u'╡',	// 0x2561,	53	5	(181)
		u'╢',	// 0x2562,	54	6	(182)
		u'╖',	// 0x2556,	55	7	(183)
		u'╕',	// 0x2555,	56	8	(184)
		u'╣',	// 0x2563,	57	9	(185)
		u'║',	// 0x2551,	58	:	(186)
		u'╗',	// 0x2557,	59	;	(187)
		u'╝',	// 0x255D,	60	<	(188)
		u'╜',	// 0x255C,	61	=	(189)
		u'╛',	// 0x255B,	62	>	(190)
		u'┐',	// 0x2510,	63	?	(191)
		u'└',	// 0x2514,	64	@	(192)
		u'┴',	// 0x2534,	65	A	(193)
		u'┬',	// 0x252C,	66	B	(194)
		u'├',	// 0x251C,	67	C	(195)
		u'─',	// 0x2500,	68	D	(196)
		u'┼',	// 0x253C,	69	E	(197)
		u'╞',	// 0x255E,	70	F	(198)
		u'╟',	// 0x255F,	71	G	(199)
		u'╚',	// 0x255A,	72	H	(200)
		u'╔',	// 0x2554,	73	I	(201)
		u'╩',	// 0x2569,	74	J	(202)
		u'╦',	// 0x2566,	75	K	(203)
		u'╠',	// 0x2560,	76	L	(204)
		u'═',	// 0x2550,	77	M	(205)
		u'╬',	// 0x256C,	78	N	(206)
		u'╧',	// 0x2567,	79	O	(207)
		u'╨',	// 0x2568,	80	P	(208)
		u'╤',	// 0x2564,	81	Q	(209)
		u'╥',	// 0x2565,	82	R	(210)
		u'╙',	// 0x2559,	83	S	(211)
		u'╘',	// 0x2558,	84	T	(212)
		u'╒',	// 0x2552,	85	U	(213)
		u'╓',	// 0x2553,	86	V	(214)
		u'╫',	// 0x256B,	87	W	(215)
		u'╪',	// 0x256A,	88	X	(216)
		u'┘',	// 0x2518,	89	Y	(217)
		u'┌',	// 0x250C,	90	Z	(218)
		u'█',	// 0x2588,	91	[	(219)
		u'▄',	// 0x2584,	92	\	(220)
		u'▌',	// 0x258C,	93	]	(221)
		u'▐',	// 0x2590,	94	^	(222)
		u'▀',	// 0x2580,	95	_	(223)
		u'↕',	// 0x2195,	96	`	(18)  - moved from low DOS ASCII
		u'↑',	// 0x2191,	97	a	(24)
		u'↓',	// 0x2193,	98	b	(25)
		u'→',	// 0x2192,	99	c	(26)
		u'←',	// 0x2190,	100	d	(27)
		u'↔',	// 0x2194,	101	e	(29)
		u'▲',	// 0x25B2,	102	f	(30)
		u'▼',	// 0x25BC,	103	g	(31)
		u'►',	// 0x25BA,	104	h	(16)
		u'◄',	// 0x25C4,	105	i	(17)
		u'◢',	// 0x25E2,	106	j	- added for slanted corners
		u'◣',	// 0x25E3,	107	k
		u'◤',	// 0x25E4,	108	l
		u'◥',	// 0x25E5,	109	m
		u'╭',	// 0x256D,	110	n	- rounded corners
		u'╮',	// 0x256E,	111	o
		u'╯',	// 0x256F,	112	p
		u'╰',	// 0x2570,	113	q
		u'▖',	// 0x259,	114	r
		u'▗',	// 0x259,	115	s
		u'▘',	// 0x259,	116	t
		u'▙',	// 0x259,	117	u
		u'▚',	// 0x259,	118	v
		u'▛',	// 0x259,	119	w
		u'▜',	// 0x259,	120	x
		u'▝',	// 0x259,	121	y
		u'▞',	// 0x259,	122	z
		u'▟',	// 0x259,	123	{
		0,		// 0x0,  	124	|	- reserved
		u'✔',	// 0x2714,	125	}	- checkboxes or checklist items
		u'✘',	// 0x2718,	126	~
		// %%END:1%%
	};

#endif //ESPTERM_CHARACTER_SETS_H_H
