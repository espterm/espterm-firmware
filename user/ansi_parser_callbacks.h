#ifndef ANSI_PARSER_CALLBACKS_H
#define ANSI_PARSER_CALLBACKS_H

void apars_handle_badseq(void);
void apars_handle_CSI(char leadchar, int *params, char keychar);
void apars_handle_RESET_cmd(void);
void apars_handle_plainchar(char c);
void apars_handle_OSC_FactoryReset(void);
void apars_handle_OSC_SetScreenSize(int rows, int cols);
void apars_handle_saveCursorAttrs(void);
void apars_handle_restoreCursorAttrs(void);

#endif //ESP_VT100_FIRMWARE_ANSI_PARSER_CALLBACKS_H
