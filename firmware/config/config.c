#include "config.h"

void configure_processor(void)
{
}

void configure_ports(void)
{
	/* ATmega32 UART: PD1 = TX (out), PD0 = RX (in) — breadboard / CP2102 */
	DDRD |= (1 << PD1);
	DDRD &= ~(1 << PD0);
	PORTD |= (1 << PD0); /* optional pull-up on RX */
}

void configure_uart(void)
{
	UBRRL = (uint8_t)RATE;
#if defined(URSEL)
	UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0); /* 8N1 */
#else
	UCSRC = (1 << UCSZ1) | (1 << UCSZ0);
#endif
	/* Polling harness: no RX/TX interrupts */
	UCSRB = (1 << RXEN) | (1 << TXEN);
}
