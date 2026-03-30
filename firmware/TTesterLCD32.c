/*
 * comms-testing harness — UART only (breadboard + USB-serial e.g. CP2102).
 * configure_ports / configure_processor / configure_uart match full firmware;
 * minimal USART ISRs so RXCIE/TXCIE are safe without the rest of the app.
 */
#include <avr/io.h>
#include <avr/interrupt.h>

#include "config/config.h"
#include "communication/communication.h"

ISR(USART_TXC_vect)
{
	/* Full firmware pushes EVENT_UART_TXC; harness has nothing to chain. */
}

ISR(USART_RXC_vect)
{
	uint8_t b = UDR;
	char2rs(b);
}

int main(void)
{
	configure_processor();
	configure_ports();
	configure_uart();

	sei();

	cstr2rs("comms-test ready\r\n");

	for (;;)
		;
}
