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

/**
 * Send a response to UART0
 * @param str
 */
void ICACHE_FLASH_ATTR
apars_respond(const char *str)
{
	UART_SendAsync(str, -1);
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
	// version encased in SOS and ST
	apars_respond("\x1bXESPTerm " FIRMWARE_VERSION "\x1b\\");
}

void ICACHE_FLASH_ATTR
apars_handle_tab(void)
{
	screen_tab_forward(1);
}
