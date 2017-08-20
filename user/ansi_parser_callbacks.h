#ifndef ANSI_PARSER_CALLBACKS_H
#define ANSI_PARSER_CALLBACKS_H

#include "apars_csi.h"
#include "apars_dcs.h"
#include "apars_osc.h"
#include "apars_string.h"
#include "apars_short.h"
#include "apars_utf8.h"

void apars_respond(const char *str);

void apars_handle_bel(void);
void apars_handle_enq(void);
void apars_handle_tab(void);

extern void apars_show_context(void);

#endif //ESP_VT100_FIRMWARE_ANSI_PARSER_CALLBACKS_H
