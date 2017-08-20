//
// Created by MightyPork on 2017/08/20.
//

#ifndef ESP_VT100_FIRMWARE_APARS_SIMPLE_H
#define ESP_VT100_FIRMWARE_APARS_SIMPLE_H

void apars_handle_short_cmd(char c);
void apars_handle_hash_cmd(char c);
void apars_handle_space_cmd(char c);
void apars_handle_chs_designate(char slot, char c);
void apars_handle_chs_switch(int Gx);

#endif //ESP_VT100_FIRMWARE_APARS_SIMPLE_H
