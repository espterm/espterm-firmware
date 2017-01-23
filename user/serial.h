#ifndef SERIAL_H
#define SERIAL_H

/** Init the uarts */
void serialInit(void);

void UART_HandleRxByte(char c);

#endif //SERIAL_H
