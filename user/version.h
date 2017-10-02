//
// Created by MightyPork on 2017/08/20.
//

#ifndef ESP_VT100_FIRMWARE_VERSION_H
#define ESP_VT100_FIRMWARE_VERSION_H

#include "helpers.h"

#define FW_V_MAJOR 2
#define FW_V_MINOR 1
#define FW_V_PATCH 0
#define FW_V_SUFFIX "-beta"
//#define FW_V_SUFFIX ""
#define FW_CODENAME "Anthill" // 2.1.0
#define FW_CODENAME_QUOTED "\""FW_CODENAME"\""

#define FW_VERSION STR(FW_V_MAJOR) "." STR(FW_V_MINOR) "." STR(FW_V_PATCH) FW_V_SUFFIX

#define VERSION_STRING FW_VERSION " " FW_CODENAME_QUOTED

#define FIRMWARE_VERSION_NUM (FW_V_MAJOR*1000 + FW_V_MINOR*10 + FW_V_PATCH) // this is used in ID queries

#define TERMINAL_GITHUB_REPO_NOPROTO "github.com/espterm/espterm-firmware"
#define TERMINAL_GITHUB_REPO "https://"TERMINAL_GITHUB_REPO_NOPROTO

#define TERMINAL_GITHUB_REPO_FRONT "https://github.com/espterm/espterm-front-end"

#endif //ESP_VT100_FIRMWARE_VERSION_H
