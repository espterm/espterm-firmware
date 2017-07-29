/**
 * Higher level UART driver that makes os_printf work and supports async Rx on UART0
 */

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain
 * this notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include <esp8266.h>
#include "uart_driver.h"
#include "uart_handler.h"

// messy irq/task based UART handling below

static void uart0_rx_intr_handler(void *para);
static void uart_recvTask(os_event_t *events);

#define uart_recvTaskPrio        1
#define uart_recvTaskQueueLen    10
static os_event_t uart_recvTaskQueue[uart_recvTaskQueueLen];

/** Clear the fifos */
void ICACHE_FLASH_ATTR clear_rxtx(int uart_no)
{
	SET_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);
	CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);
}


/** Configure basic UART func and pins */
void ICACHE_FLASH_ATTR UART_Init(void)
{
	// U0TXD
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0TXD_U);

	// U0RXD
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_U0RXD);

	// U1TXD (GPIO2)
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_U1TXD_BK);

	// Configure the UART peripherals
	UART_SetWordLength(UART0, EIGHT_BITS); // main
	UART_ResetFifo(UART0);
	UART_SetWordLength(UART1, EIGHT_BITS); // debug (output only)
	UART_ResetFifo(UART1);
}

/** Configure Rx on UART0 */
void ICACHE_FLASH_ATTR UART_SetupAsyncReceiver(void)
{
	// Start the Rx reading task
	system_os_task(uart_recvTask, uart_recvTaskPrio, uart_recvTaskQueue, uart_recvTaskQueueLen);
	// set handler
	ETS_UART_INTR_ATTACH((void *)uart0_rx_intr_handler, &(UartDev.rcv_buff)); // the buf will be used as an arg

	// fifo threshold config
	uint32_t conf = ((100 & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S);
	conf |= ((0x10 & UART_TXFIFO_EMPTY_THRHD) << UART_TXFIFO_EMPTY_THRHD_S);
	// timeout config
	conf |= ((0x02 & UART_RX_TOUT_THRHD) << UART_RX_TOUT_THRHD_S); // timeout threshold
	conf |= UART_RX_TOUT_EN; // enable timeout
	WRITE_PERI_REG(UART_CONF1(UART0), conf);

	// enable TOUT and ERR irqs
	SET_PERI_REG_MASK(UART_INT_ENA(UART0), UART_RXFIFO_TOUT_INT_ENA | UART_FRM_ERR_INT_ENA);
	/* clear interrupt flags */
	WRITE_PERI_REG(UART_INT_CLR(UART0), 0xffff);
	/* enable RX interrupts */
	SET_PERI_REG_MASK(UART_INT_ENA(UART0), UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_OVF_INT_ENA);

	// Enable IRQ in Extensa
	ETS_UART_INTR_ENABLE();
}


// ---- async receive stuff ----


void uart_rx_intr_disable(uint8 uart_no)
{
	CLEAR_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_TOUT_INT_ENA);
}


void uart_rx_intr_enable(uint8 uart_no)
{
	SET_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_TOUT_INT_ENA);
}


/**
 * @brief get number of bytes in UART tx fifo
 * @param UART number
 */
#define UART_GetRxFifoCount(uart_no) ((READ_PERI_REG(UART_STATUS((uart_no))) >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT)


void ICACHE_FLASH_ATTR UART_PollRx(void)
{
	uint8 fifo_len = UART_GetRxFifoCount(UART0);

	for (uint8 idx = 0; idx < fifo_len; idx++) {
		uint8 d_tmp = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
		UART_HandleRxByte(d_tmp);
	}
}


static void ICACHE_FLASH_ATTR uart_recvTask(os_event_t *events)
{
	if (events->sig == 0) {
		UART_PollRx();

		// clear irq flags
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR | UART_RXFIFO_TOUT_INT_CLR);
		// enable rx irq again
		uart_rx_intr_enable(UART0);
	} else if (events->sig == 1) {
		// ???
	}
}


/******************************************************************************
 * FunctionName : uart0_rx_intr_handler
 * Description  : Internal used function
 *                UART0 interrupt handler, add self handle code inside
 * Parameters   : void *para - point to ETS_UART_INTR_ATTACH's arg
 * Returns      : NONE
*******************************************************************************/
static void
uart0_rx_intr_handler(void *para)
{
	(void)para;

	uint32_t status_reg = READ_PERI_REG(UART_INT_ST(UART0));

	if (status_reg & UART_FRM_ERR_INT_ST) {
		// Framing Error
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_FRM_ERR_INT_CLR);
	}

	if (status_reg & UART_RXFIFO_FULL_INT_ST) {
		// RX fifo full
		uart_rx_intr_disable(UART0);
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR);

		// run handler
		system_os_post(uart_recvTaskPrio, 0, 0); /* -> notify the polling thread */
	}

	if (status_reg & UART_RXFIFO_TOUT_INT_ST) {
		// Fifo timeout
		uart_rx_intr_disable(UART0);
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_TOUT_INT_CLR);

		// run handler
		system_os_post(uart_recvTaskPrio, 0, 0); /* -> notify the polling thread */
	}

	if (status_reg & UART_TXFIFO_EMPTY_INT_ST) {
		CLEAR_PERI_REG_MASK(UART_INT_ENA(UART0), UART_TXFIFO_EMPTY_INT_ENA);
	}

	if (status_reg & UART_RXFIFO_OVF_INT_ST) {
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_OVF_INT_CLR);
	}
}
