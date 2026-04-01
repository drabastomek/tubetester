/*
 * comms-testing harness — VTTester protocol v0.4.1 (binary, 9-byte frames) on USART.
 * ISRs stay here; communication layer is byte-oriented (comm_receive_byte).
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
	comm_receive_byte(UDR);
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
	 * Send debug line before sei()
	 */
#ifdef COMMS_DEBUG_BOOT
	comm_transmit_string("comms v0.4.1 binary\r\n");
#endif

	sei();

	for (;;)
		comm_handle_requests();
}
