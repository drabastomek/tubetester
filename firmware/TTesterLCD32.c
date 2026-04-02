/*
 * comms-testing harness — VTTester protocol v0.4.1 (binary, 9-byte frames) on USART.
 * ISRs stay here; communication layer is byte-oriented (comm_receive_byte).
 *
 * Double-triode style host flow (see protocol §9, user sequence):
 *   SET sys1 … → ACK (heat/settle) → RESULT (3× DATA) → SET sys1 other Ug1 → …
 *   RESET Ua,Ug1 → SET sys2 … → … → RESET Ua,Ug1,Uh → swap tube → repeat.
 *
 * DATA is not sent in the same comm_handle_requests pass as ACK: main calls
 * harness_enqueue_measurement_stub() first so the host sees ACK before RESULT.
 */
#include <avr/io.h>
#include <avr/interrupt.h>

#include "config/config.h"
#include "communication/communication.h"

/* Port: USART TX for comm layer (communication.c is MCU/compiler-agnostic). */
void comm_transmit_byte(uint8_t b)
{
	while (!(UCSRA & (1u << UDRE)))
		;
	UDR = b;
}

ISR(USART_TXC_vect)
{
}

ISR(USART_RXC_vect)
{
	comm_receive_byte(UDR);
}

/* ------------------------------------------------------------------------- */
/* 1 ms tick (Timer0 CTC) — SET→DATA delay uses P6 × HARNESS_P6_MS_PER_UNIT */
/* ------------------------------------------------------------------------- */

/** Test harness only: P6 maps to delay as P6 × this many ms (spec prod. uses 500 ms steps). */
#define HARNESS_P6_MS_PER_UNIT 100u

static volatile uint32_t harness_ms_tick;

static uint32_t harness_ms_now(void)
{
	uint32_t t;
	uint8_t sreg;

	sreg = SREG;
	cli();
	t = harness_ms_tick;
	SREG = sreg;
	return t;
}

ISR(TIMER0_COMP_vect)
{
	harness_ms_tick++;
}

static void harness_timer0_1ms_init(void)
{
	/* 16 MHz / 64 / (1+249) = 1000 Hz */
	TCCR0 = (uint8_t)((1u << WGM01) | (1u << CS01) | (1u << CS00));
	OCR0 = 249u;
	TCNT0 = 0u;
	TIFR = (uint8_t)(1u << OCF0);
	TIMSK |= (uint8_t)(1u << OCIE0);
}

/* ------------------------------------------------------------------------- */
/* Harness PRNG + nominal × random band (permille = factor × 1000) */
/* ------------------------------------------------------------------------- */

static uint32_t harness_rng_state = 0xC001D00Du;

static uint32_t harness_prng_u32(void)
{
	harness_rng_state = harness_rng_state * 1664525u + 1013904223u;
	return harness_rng_state;
}

/** Uniform 0 .. n-1 (n must be > 0). */
static uint16_t harness_prng_below(uint16_t n)
{
	return (uint16_t)(harness_prng_u32() % (uint32_t)n);
}

/**
 * scaled = floor(base_nom * f / 1000), f integer in [lo_permille, hi_permille], clamped to uint16.
 * Example: lo=980 hi=1030 → f in [0.980, 1.030].
 */
static uint16_t harness_scale_permille(uint32_t base_nom, uint16_t lo_permille, uint16_t hi_permille)
{
	uint16_t span;
	uint16_t f;
	uint32_t p;

	span = (uint16_t)(hi_permille - lo_permille + 1u);
	f = (uint16_t)(lo_permille + harness_prng_below(span));
	p = base_nom * (uint32_t)f / 1000u;
	if (p > 65535u)
		p = 65535u;
	return (uint16_t)p;
}

/**
 * Toy ADC sums from last remote SET, with multiplicative spread vs nominal.
 * Nominally each channel's uint16 is the sum of 64 samples; host divides by 64 (§10.1).
 * Scale noms so that mean ≈ SET parameter in spec units: Ua/Ug2 ≈ volts (12-bit),
 * P1/P5 ≈ 0.1 V / 10 mA steps, Ug2=0 → 0 (not a fake bias).
 *   Ua  × [0.98, 1.03]   Ug2 × [0.98, 1.03]   Ug1 × [0.98, 1.03]
 *   Uh  × [0.95, 1.05]   Ih  × [0.95, 1.05]   (heater pair)
 *   Ia  × [0.75, 1.15]   Ig2 × [0.75, 1.15]
 *
 * DATA is delayed by P6 × HARNESS_P6_MS_PER_UNIT ms after ACK (harness only).
 * Main calls comm_handle_requests() first each loop so ACK/STATUS drain before the
 * harness arms the P6 deadline — no delay logic inside communication/.
 */
static uint8_t harness_meas_delay_armed;
static uint32_t harness_meas_deadline_ms;

static void harness_try_enqueue_measurement(void)
{
	uint8_t idx;
	uint8_t p1;
	uint16_t ua12;
	uint16_t ug212;
	uint8_t p5;
	uint8_t p6;
	uint32_t ua_nom;
	uint32_t ug2_nom;
	uint32_t uh_nom;
	uint32_t ih_nom;
	uint32_t ug1_nom;
	uint16_t ih_sum;
	uint16_t ua_sum;
	uint16_t ug2_sum;
	uint16_t uh_sum;
	uint16_t ug1_sum;
	uint16_t ia_nom;
	uint16_t ig2_nom;
	uint16_t ia_sum;
	uint16_t ig2_sum;
	uint32_t now;

	if (!comm_measurement_data_pending())
	{
		harness_meas_delay_armed = 0;
		return;
	}

	comm_get_last_remote_set(&idx, &p1, &ua12, &ug212, &p5, &p6);
	now = harness_ms_now();

	if (!harness_meas_delay_armed)
	{
		harness_meas_deadline_ms = now + (uint32_t)p6 * HARNESS_P6_MS_PER_UNIT;
		harness_meas_delay_armed = 1;
	}

	if (now < harness_meas_deadline_ms)
		return;

	/* Stir RNG per measurement so consecutive SETs differ. */
	harness_rng_state += (uint32_t)idx + (uint32_t)ua12 + (uint32_t)ug212 + (uint32_t)p5 + (uint32_t)p1;

	uh_nom = (uint32_t)p1 * 64u;
	ih_nom = (uint32_t)p1 * 64u;
	ua_nom = (uint32_t)ua12 * 64u;
	ug2_nom = (uint32_t)ug212 * 64u;
	ug1_nom = (uint32_t)p5 * 64u;
	ia_nom = 9600u;
	ig2_nom = 640u;

	uh_sum = harness_scale_permille(uh_nom, 950u, 1050u);
	ih_sum = harness_scale_permille(ih_nom, 950u, 1050u);
	ua_sum = harness_scale_permille(ua_nom, 980u, 1030u);
	ug2_sum = harness_scale_permille(ug2_nom, 980u, 1030u);
	ug1_sum = harness_scale_permille(ug1_nom, 980u, 1030u);
	ia_sum = harness_scale_permille((uint32_t)ia_nom, 750u, 1150u);
	ig2_sum = harness_scale_permille((uint32_t)ig2_nom, 750u, 1150u);

	comm_enqueue_measurement_data(
	    idx,
	    uh_sum,
	    ih_sum,
	    ua_sum,
	    ug2_sum,
	    ug1_sum,
	    ia_sum,
	    ig2_sum,
	    0u);

	comm_clear_measurement_data_pending();
	harness_meas_delay_armed = 0;
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

#ifdef COMMS_DEBUG_BOOT
	comm_transmit_string("comms v0.4.1 binary\r\n");
#endif

	harness_timer0_1ms_init();
	sei();

	for (;;)
	{
		comm_handle_requests();
		harness_try_enqueue_measurement();
	}
}
