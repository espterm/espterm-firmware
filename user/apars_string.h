//
// Created by MightyPork on 2017/08/20.
//

#ifndef ESP_VT100_FIRMWARE_APARS_STRING_H
#define ESP_VT100_FIRMWARE_APARS_STRING_H

// not const char so some edits can be made when processing
void apars_handle_string_cmd(char leadchar, char *buffer);

#endif //ESP_VT100_FIRMWARE_APARS_STRING_H
