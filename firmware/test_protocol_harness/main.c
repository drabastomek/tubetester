/*
 * VTTester v0.5 protocol test harness — ATmega32A UART stub.
 * Build: make -C firmware  |  Flash: make -C firmware flash
 */
#include <avr/io.h>
#include <avr/interrupt.h>

#include "config/config.h"
#include "communication/communication.h"
#include "harness_command.h"
#include "harness_tick.h"
#include "scenario.h"
#include "synthetic_measure.h"

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
	harness_tick_init();
	synthetic_measure_init();
	scenario_init();
	sei();

	for (;;) {
		uint8_t *rq;

		scenario_poll();

		rq = comm_rx_msg();
		if (rq)
			harness_command_handle(rq);
	}
}
