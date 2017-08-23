/**
 * UART init & async rx module.
 *
 * Call UART_Init(), UART_SetupAsyncReceiver() and
 * define UART_HandleRxByte() somewhere in application code.
 *
 * Call UART_PollRx() to allow rx in a blocking handler.
 */

#ifndef UART_HANDLER_H
#define UART_HANDLER_H

#include <esp8266.h>

/** Configure UART periphs and enable pins - does not set baud rate, parity and stopbits */
void UART_Init(void);

/** Configure async Rx on UART0 */
void UART_SetupAsyncReceiver(void);

///** User must provide this func for handling received bytes */
extern void UART_HandleRxByte(char c);

void uart_rx_intr_disable(uint8 uart_no);

void uart_rx_intr_enable(uint8 uart_no);

#endif
