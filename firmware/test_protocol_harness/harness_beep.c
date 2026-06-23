#include <avr/io.h>
#include <util/delay.h>

#include "communication/communication.h"
#include "config/config.h"
#include "harness_beep.h"

void harness_beep_play(uint8_t dur_units)
{
	uint8_t i;
	uint8_t units;

	units = dur_units;
	if (units == 0u)
		units = BEEP_DUR_DEFAULT;

	PORTD |= BIT(7);
	for (i = 0; i < units; i++)
		_delay_ms(BEEP_DUR_UNIT_MS);
	PORTD &= ~BIT(7);
}
