#ifndef ANSI_PARSER_H
#define ANSI_PARSER_H

#include <stdlib.h>

extern volatile bool ansi_parser_inhibit; // discard all characters

void ansi_parser_reset(void);

extern volatile u32 ansi_parser_char_cnt;

/**
 * \brief Linear ANSI chars stream parser
 * 
 * Parses a stream of bytes using a Ragel parser. The defined
 * grammar does not use 'unget', so the entire buffer is 
 * always processed in a linear manner.
 * 
 * \attention -> but always check the Ragel output for 'p--' 
 *            or 'p -=', that means trouble.
 * 
 * \param newchar - received char
 */
void ansi_parser(char newchar);

/** This shows a short error message and prints the history (if any) */
void apars_show_context(void);

#endif // ANSI_PARSER_H
