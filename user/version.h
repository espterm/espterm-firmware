//
// Created by MightyPork on 2017/08/20.
//

#ifndef ESP_VT100_FIRMWARE_VERSION_H
#define ESP_VT100_FIRMWARE_VERSION_H

#define FW_V_MAJOR 8
#define FW_V_MINOR 0
#define FW_V_PATCH 0

#define FIRMWARE_VERSION STR(FW_V_MAJOR) "." STR(FW_V_MINOR) "." STR(FW_V_PATCH) "+" GIT_HASH
#define FIRMWARE_VERSION_NUM (FW_V_MAJOR*10000 + FW_V_MINOR*100 + FW_V_PATCH) // this is used in ID queries
#define TERMINAL_GITHUB_REPO "https://github.com/MightyPork/ESPTerm"

#endif //ESP_VT100_FIRMWARE_VERSION_H
