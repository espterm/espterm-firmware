//
// Created by MightyPork on 2017/10/01.
//

#ifndef ESPTERM_CGI_D2D_H
#define ESPTERM_CGI_D2D_H

#include <esp8266.h>

#define D2D_MSG_ENDPOINT "/api/v1/msg"

bool d2d_parse_command(char *msg);

#endif //ESPTERM_CGI_D2D_H
