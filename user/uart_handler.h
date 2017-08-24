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

/** User must provide this func for handling received bytes */
extern void UART_HandleRxByte(char c);

static inline void uart_rx_intr_disable(uint8 uart_no)
{
	CLEAR_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_TOUT_INT_ENA);
}

static inline void uart_rx_intr_enable(uint8 uart_no)
{
	SET_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_TOUT_INT_ENA);
}

/**
 * @brief get number of bytes in UART tx fifo
 * @param UART number
 */
static inline u8 UART_GetRxFifoCount(u8 uart_no) {
	return (u8) ((READ_PERI_REG(UART_STATUS((uart_no))) >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT);
}

#endif
