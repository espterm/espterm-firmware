/*
* Driver file for ESP8266 UART, works with the SDK.
*/

#include "uart_driver.h"

#include <esp8266.h>

#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "os_type.h"

#include "ets_sys_extra.h"
#include "uart_register.h"

//========================================================


void ICACHE_FLASH_ATTR UART_SetWordLength(UARTn uart_no, UartBitsNum4Char len)
{
	SET_PERI_REG_BITS(UART_CONF0(uart_no), UART_BIT_NUM, len, UART_BIT_NUM_S);
}


void ICACHE_FLASH_ATTR UART_SetStopBits(UARTn uart_no, UartStopBitsNum bit_num)
{
	SET_PERI_REG_BITS(UART_CONF0(uart_no), UART_STOP_BIT_NUM, bit_num, UART_STOP_BIT_NUM_S);
}


void ICACHE_FLASH_ATTR UART_SetLineInverse(UARTn uart_no, UART_LineLevelInverse inverse_mask)
{
	CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_LINE_INV_MASK);
	SET_PERI_REG_MASK(UART_CONF0(uart_no), inverse_mask);
}


void ICACHE_FLASH_ATTR UART_SetParity(UARTn uart_no, UartParityMode Parity_mode)
{
	CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_PARITY | UART_PARITY_EN);
	if (Parity_mode == PARITY_NONE) {
	} else {
		SET_PERI_REG_MASK(UART_CONF0(uart_no), Parity_mode | UART_PARITY_EN);
	}
}


void ICACHE_FLASH_ATTR UART_SetBaudrate(UARTn uart_no, uint32 baud_rate)
{
	uart_div_modify(uart_no, UART_CLK_FREQ / baud_rate);
}


void ICACHE_FLASH_ATTR UART_SetFlowCtrl(UARTn uart_no, UART_HwFlowCtrl flow_ctrl, uint8 rx_thresh)
{
	if (flow_ctrl & USART_HWFlow_RTS) {
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_U0RTS);
		SET_PERI_REG_BITS(UART_CONF1(uart_no), UART_RX_FLOW_THRHD, rx_thresh, UART_RX_FLOW_THRHD_S);
		SET_PERI_REG_MASK(UART_CONF1(uart_no), UART_RX_FLOW_EN);
	} else {
		CLEAR_PERI_REG_MASK(UART_CONF1(uart_no), UART_RX_FLOW_EN);
	}

	if (flow_ctrl & USART_HWFlow_CTS) {
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_UART0_CTS);
		SET_PERI_REG_MASK(UART_CONF0(uart_no), UART_TX_FLOW_EN);
	} else {
		CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_TX_FLOW_EN);
	}
}


void ICACHE_FLASH_ATTR UART_WaitTxFifoEmpty(UARTn uart_no , uint32 time_out_us) //do not use if tx flow control enabled
{
	uint32 t_s = system_get_time();
	while (READ_PERI_REG(UART_STATUS(uart_no)) & (UART_TXFIFO_CNT << UART_TXFIFO_CNT_S)) {

		if ((system_get_time() - t_s) > time_out_us) {
			break;
		}

		system_soft_wdt_feed();
	}
}


bool ICACHE_FLASH_ATTR UART_CheckOutputFinished(UARTn uart_no, uint32 time_out_us)
{
	uint32 t_start = system_get_time();
	uint8 tx_fifo_len;

	while (1) {
		tx_fifo_len = UART_TxQueLen(uart_no);

		// TODO If using output circbuf, check if empty

		if (tx_fifo_len == 0) {
			return TRUE;
		}

		if (system_get_time() - t_start > time_out_us) {
			return FALSE;
		}

		system_soft_wdt_feed();
	}
}


void ICACHE_FLASH_ATTR UART_ResetFifo(UARTn uart_no)
{
	SET_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);
	CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);
}


void ICACHE_FLASH_ATTR UART_ClearIntrStatus(UARTn uart_no, uint32 clr_mask)
{
	WRITE_PERI_REG(UART_INT_CLR(uart_no), clr_mask);
}


void ICACHE_FLASH_ATTR UART_SetIntrEna(UARTn uart_no, uint32 ena_mask)
{
	SET_PERI_REG_MASK(UART_INT_ENA(uart_no), ena_mask);
}


LOCAL void u0_putc_crlf(char c)
{
	UART_WriteCharCRLF(UART0, (u8)c, UART_TIMEOUT_US);
}


LOCAL void u1_putc_crlf(char c)
{
	UART_WriteCharCRLF(UART1, (u8)c, UART_TIMEOUT_US);
}


void ICACHE_FLASH_ATTR UART_SetPrintPort(UARTn uart_no)
{
	if (uart_no == UART0) {
		os_install_putc1((void *)u0_putc_crlf);
	} else {
		os_install_putc1((void *)u1_putc_crlf);
	}
}


// -------------- Custom UART functions -------------------------

// !!! write handlers are not ICACHE_FLASH_ATTR -> can be used in IRQ !!!

/**
 * @brief Write a char to UART.
 * @param uart_no
 * @param c
 * @param timeout_us - how long to max wait for space in FIFO.
 * @return write success
 */
STATUS UART_WriteChar(UARTn uart_no, uint8 c, uint32 timeout_us)
{
	if (timeout_us == 0) {
		timeout_us = UART_TIMEOUT_US;
	}


	uint32 t_s = system_get_time();

	while ((system_get_time() - t_s) < timeout_us) {
		uint8 fifo_cnt = UART_TxQueLen(uart_no);

		if (fifo_cnt < UART_TX_FULL_THRESH_VAL) {
			WRITE_PERI_REG(UART_FIFO(uart_no), c);
			return OK;
		}

		system_soft_wdt_feed();
	}

	return FAIL;
}


/**
 * @brief Write a char to UART, translating LF to CRLF and discarding CR.
 * @param uart_no
 * @param c
 * @param timeout_us - how long to max wait for space in FIFO.
 * @return write success
 */
STATUS UART_WriteCharCRLF(UARTn uart_no, uint8 c, uint32 timeout_us)
{
	STATUS st;

	if (c == '\r') {
		return OK;
	} else if (c == '\n') {

		st = UART_WriteChar(uart_no, '\r', timeout_us);
		if (st != OK) return st;

		st = UART_WriteChar(uart_no, '\n', timeout_us);
		return st;

	} else {
		return UART_WriteChar(uart_no, c, timeout_us);
	}
}


/**
 * @brief Send a string to UART.
 * @param uart_no
 * @param str
 * @param timeout_us - how long to max wait for space in FIFO.
 * @return write success
 */
STATUS UART_WriteString(UARTn uart_no, const char *str, uint32 timeout_us)
{
	while (*str) {
		STATUS suc = UART_WriteChar(uart_no, (u8) * str++, timeout_us);
		if (suc != OK) return suc;
	}

	return OK;
}



/**
 * @brief Send a buffer
 * @param uart_no
 * @param buffer - buffer to send
 * @param len - buffer size
 * @param timeout_us - how long to max wait for space in FIFO.
 * @return write success
 */
STATUS UART_WriteBuffer(UARTn uart_no, const uint8 *buffer, size_t len, uint32 timeout_us)
{
	for (size_t i = 0; i < len; i++) {
		STATUS suc = UART_WriteChar(uart_no, (u8) * buffer++, timeout_us);
		if (suc != OK) return suc;
	}

	return OK;
}
