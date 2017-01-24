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

/** Configure UART periphs and enable pins */
void UART_Init(uint32_t baud0, uint32_t baud1);

/** Configure async Rx on UART0 */
void UART_SetupAsyncReceiver(void);

/** User must provide this func for handling received bytes */
extern void UART_HandleRxByte(char c);

/** Poll uart manually while waiting for something */
void UART_PollRx(void);

#endif
