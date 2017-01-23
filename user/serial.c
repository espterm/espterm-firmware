#include <esp8266.h>
#include "uart_driver.h"
#include "uart_handler.h"
#include "ansi_parser.h"

// Here the bitrates are defined
#define UART0_BAUD BIT_RATE_115200
#define UART1_BAUD BIT_RATE_115200

/**
 * Init the serial ports
 */
void ICACHE_FLASH_ATTR serialInit(void)
{
	UART_Init(UART0_BAUD, UART1_BAUD);
	UART_SetPrintPort(UART0);
	UART_SetupAsyncReceiver();
}

/**
 * Handle a byte received from UART.
 * Might do some buffering here maybe
 *
 * @param c
 */
void ICACHE_FLASH_ATTR UART_HandleRxByte(char c)
{
	if (c > 0 && c < 127) {
		// TODO buffering, do not run parser after just 1 char
		ansi_parser(&c, 1);
	} else {
		warn("Bad char %d ('%c')", (unsigned char)c, c);
	}
}