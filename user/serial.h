#ifndef SERIAL_H
#define SERIAL_H

/** Initial uart init (before sysconf is loaded) */
void serialInitBase(void);

/** Init the uarts */
void serialInit(void);

void UART_HandleRxByte(char c);

#endif //SERIAL_H
