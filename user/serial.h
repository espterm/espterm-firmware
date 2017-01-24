#ifndef STDOUT_H
#define STDOUT_H

#include <esp8266.h>

/** Init the uarts */
void serialInit();

/** poll uart while waiting for something */
void uart_poll(void);

#endif
