#include <avr/io.h>
#include <avr/interrupt.h>

#include "harness_config.h"
#include "harness_tick.h"

static volatile uint16_t s_millis;

ISR(TIMER0_COMP_vect)
{
	s_millis++;
}

void harness_tick_init(void)
{
	s_millis = 0u;
	TCCR0 = BIT(WGM01);
	TCCR0 |= BIT(CS01) | BIT(CS00);
	OCR0 = (uint8_t)((F_CPU / 64UL / 1000UL) * HARNESS_TICK_MS - 1UL);
	TIMSK = BIT(OCIE0);
}

uint16_t harness_millis(void)
{
	uint16_t now;

	cli();
	now = s_millis;
	sei();
	return now;
}
