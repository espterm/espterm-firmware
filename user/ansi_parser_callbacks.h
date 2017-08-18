#ifndef ANSI_PARSER_CALLBACKS_H
#define ANSI_PARSER_CALLBACKS_H

#include "screen.h"

void apars_handle_plainchar(char c);
void apars_handle_CSI(char leadchar, int *params, int count, char keychar);
void apars_handle_OSC_SetScreenSize(int rows, int cols);
void apars_handle_OSC_SetButton(int num, const char *buffer);
void apars_handle_OSC_SetTitle(const char *buffer);
void apars_handle_shortCode(char c);
void apars_handle_hashCode(char c);
void apars_handle_characterSet(char leadchar, char c);
void apars_handle_setXCtrls(char c);
void apars_reset_utf8buffer(void);
void apars_handle_bel(void);

#endif //ESP_VT100_FIRMWARE_ANSI_PARSER_CALLBACKS_H
