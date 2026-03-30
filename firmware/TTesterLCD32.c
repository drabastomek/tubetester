/*
 * comms-testing harness — UART only (breadboard + USB-serial e.g. CP2102).
 * Full firmware lives on another branch; shareable surface is communication/.
 */
#include <avr/io.h>

#include "config/config.h"
#include "communication/communication.h"

int main(void)
{
	configure_processor();
	configure_ports();
	configure_uart();

	cstr2rs("comms-test ready\r\n");

	for (;;)
	{
		if (UCSRA & (1 << RXC))
		{
			uint8_t b = UDR;
			char2rs(b);
		}
	}
}
