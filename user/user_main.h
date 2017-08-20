#ifndef USER_MAIN_H_H
#define USER_MAIN_H_H

#define FW_V_MAJOR 0
#define FW_V_MINOR 6
#define FW_V_PATCH 7

#define FIRMWARE_VERSION STR(FW_V_MAJOR) "." STR(FW_V_MINOR) "." STR(FW_V_PATCH) "+" GIT_HASH
#define FIRMWARE_VERSION_NUM (FW_V_MAJOR*10000 + FW_V_MINOR*100 + FW_V_PATCH) // this is used in ID queries
#define TERMINAL_GITHUB_REPO "https://github.com/MightyPork/esp-vt100-firmware"

#endif //USER_MAIN_H_H
