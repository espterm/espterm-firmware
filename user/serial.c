#include <esp8266.h>
#include "uart_driver.h"
#include "uart_handler.h"
#include "ansi_parser.h"
#include "syscfg.h"

/**
 * Init the serial ports
 */
void ICACHE_FLASH_ATTR serialInitBase(void)
{
	UART_Init();
	// debug port
	UART_SetParity(UART1, PARITY_NONE);
	UART_SetStopBits(UART1, ONE_STOP_BIT);
	UART_SetBaudrate(UART1, BIT_RATE_115200);
	UART_SetPrintPort(UART1);
	UART_SetupAsyncReceiver();
}

/**
 * Init the serial ports
 */
void ICACHE_FLASH_ATTR serialInit(void)
{
	UART_SetParity(UART0, (UartParityMode) sysconf->uart_parity);
	UART_SetStopBits(UART0, (UartStopBitsNum) sysconf->uart_stopbits);
	UART_SetBaudrate(UART0, sysconf->uart_baudrate);

	info("COM SERIAL: %d baud, %s parity, %s stopbit(s)",
		 sysconf->uart_baudrate,
		 (sysconf->uart_parity == PARITY_NONE ? "NONE" : (sysconf->uart_parity == PARITY_ODD ? "ODD" : "EVEN")),
		 (sysconf->uart_stopbits == ONE_STOP_BIT ? "1" : (sysconf->uart_stopbits == ONE_HALF_STOP_BIT ? "1.5" : "2"))
	);
}

/**
 * Handle a byte received from UART.
 * Might do some buffering here maybe
 *
 * @param c
 */
void ICACHE_FLASH_ATTR UART_HandleRxByte(char c)
{
	ansi_parser(c);
}
