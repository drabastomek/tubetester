/*
 * comms-testing harness — VTTester protocol v0.4 (binary) on USART.
 * ISRs stay here; communication layer is byte-oriented (comm_rx_byte).
 */
#include <avr/io.h>
#include <avr/interrupt.h>

#include "config/config.h"
#include "communication/communication.h"

ISR(USART_TXC_vect)
{
}

ISR(USART_RXC_vect)
{
	comm_rx_byte(UDR);
}

int main(void)
{
	configure_processor();
	configure_ports();
	configure_uart();
	comm_init();

	/* Drain RX FIFO (noise); USART ISRs are still masked until sei(). */
	while (UCSRA & (1 << RXC))
		(void)UDR;

	/*
	 * Send debug line before sei(): with RXCIE/TXCIE on, ISR latency can
	 * otherwise interfere with polling TX. Build: make COMMS_DEBUG_BOOT=1
	 */
#ifdef COMMS_DEBUG_BOOT
	cstr2rs("comms v0.4 binary\r\n");
#endif

	sei();

	for (;;)
		comm_tx_poll();
}
