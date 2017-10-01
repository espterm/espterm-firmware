/**
 * ANSI code parser callbacks that do the actual work.
 * Split from the parser itself for easier editing without
 * having to deal with Ragel.
 */

#include <esp8266.h>
#include <helpers.h>
#include "ansi_parser_callbacks.h"
#include "cgi_sockets.h"
#include "version.h"
#include "uart_buffer.h"
#include "screen.h"
#include "uart_driver.h"

volatile bool enquiry_suppressed = false;
ETSTimer enqTimer;
void ICACHE_FLASH_ATTR enqTimerCb(void *unused)
{
	enquiry_suppressed = false;
}

/**
 * Send a response to UART0
 * @param str
 */
void ICACHE_FLASH_ATTR
apars_respond(const char *str)
{
	UART_WriteString(UART0, str, UART_TIMEOUT_US);
	//UART_SendAsync(str, -1);
}

/**
 * Beep at the user.
 */
void ICACHE_FLASH_ATTR
apars_handle_bel(void)
{
	send_beep();
}

/**
 * Send to uart the answerback message
 */
void ICACHE_FLASH_ATTR
apars_handle_enq(void)
{
	if (enquiry_suppressed) return;

	// version encased in SOS and ST
	apars_respond("\x1bXESPTerm " FIRMWARE_VERSION "\x1b\\");

	// Throttle enquiry - this is a single-character-invoked response,
	// so it tends to happen randomly when throwing garbage at the ESP.
	// We don't want to fill the output buffer with dozens of enquiry responses
	enquiry_suppressed = true;
	TIMER_START(&enqTimer, enqTimerCb, 500, 0);
}

void ICACHE_FLASH_ATTR
apars_handle_tab(void)
{
	screen_tab_forward(1);
}
