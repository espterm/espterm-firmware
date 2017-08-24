//
// Created by MightyPork on 2017/08/24.
//

#ifndef ESP_VT100_FIRMWARE_UART_BUFFER_H
#define ESP_VT100_FIRMWARE_UART_BUFFER_H

#include <esp8266.h>

// the init func
void UART_AllocBuffers(void);

// read from rx buffer
uint16 UART_ReadAsync(char *pdata, uint16 data_len);

// write to tx buffer
void UART_SendAsync(const char *pdata, int16_t data_len);

//move data from uart fifo to rx buffer
void UART_RxFifoCollect(void);
//move data from uart tx buffer to fifo
void UART_DispatchFromTxBuffer(uint8 uart_no);

u16 UART_AsyncRxCount(void);

#endif //ESP_VT100_FIRMWARE_UART_BUFFER_H
