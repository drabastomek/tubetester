#include <avr/io.h>
#include <avr/interrupt.h>

#include "config/config.h"
#include "communication/communication.h"

typedef struct
 {
	uint8_t a1_a2;
	uint8_t uhset;
	uint8_t ihset;
	uint8_t ugset;
	uint16_t uaset;
	uint16_t ueset;
} tester_params;
tester_params tp; // params to set

// measurements to send
uint16_t mugadc; // UG
uint16_t muhadc; // UH
uint16_t mihadc; // IH
uint16_t muaadc; // UA
uint16_t miaadc; // IA
uint16_t mueadc; // UE
uint16_t mieadc; // IE
uint16_t mtmadc; // TM

// interrupts set
ISR(USART_TXC_vect)
{
}

ISR(USART_RXC_vect)
{
	comm_receive_byte(UDR);
}

// purely for testing
void sample_adc(void)
{
	mugadc = (uint16_t)(20.1f  * 64.0f);
	muhadc = (uint16_t)(126.1f * 64.0f);
	mihadc = (uint16_t)(30.5f  * 64.0f);
	muaadc = (uint16_t)(256.4f * 64.0f);
	miaadc = (uint16_t)(15.1f  * 64.0f);
	mueadc = (uint16_t)(0.0f   * 64.0f);
	mieadc = (uint16_t)(0.0f   * 64.0f);
	mtmadc = (uint16_t)(12.0f  * 64.0f);
}

uint8_t validate_parameters(const uint8_t *rq)
{
	uint16_t temp;

	/*
	 * Each check is (value < MIN || value > MAX). Wire bytes are unsigned,
	 * so when MIN is 0 the left half is always false and GCC may warn
	 * (-Wtype-limits). We keep both halves anyway: one pattern everywhere,
	 * and non-zero MIN values in config.h will work without revisiting this.
	 */

	// ug
	if (rq[2] < UG_MIN || rq[2] > UG_MAX)
	{
		send_input_range_error(PARAM_ID_UG, rq[2]);
		return 0;
	}

	// uh
	if (rq[3] < UH_MIN || rq[3] > UH_MAX)
	{
		send_input_range_error(PARAM_ID_UH, rq[3]);
		return 0;
	}

	// ih
	if (rq[4] < IH_MIN || rq[4] > IH_MAX)
	{
		send_input_range_error(PARAM_ID_IH, rq[4]);
		return 0;
	}

	// ua and ue are 16 bit values
	// ua
	temp = rq[6]; temp <<= 8; temp += rq[5];
	if (temp < UA_MIN || temp > UA_MAX)
	{
		send_input_range_error(PARAM_ID_UA, rq[6]);
		return 0;
	}

	// ue
	temp = rq[8]; temp <<= 8; temp += rq[7];
	if (temp < UE_MIN || temp > UE_MAX)
	{
		send_input_range_error(PARAM_ID_UE, rq[8]);
		return 0;
	}

	return 1;
}

// handle incoming message
static void handle_rx_msg(const uint8_t *rq)
{
	uint8_t c;

	c = rq[0];

	switch (c)
	{
	case CMD_SET:
		// TODO: validate parameters / send error if invalid
		if (!validate_parameters(rq))
		{
			return;
		}

		tp.a1_a2 = rq[1];
		tp.ugset = rq[2];
		tp.uhset = rq[3];
		tp.ihset = rq[4];
		tp.uaset = rq[6]; tp.uaset <<= 8; tp.uaset += rq[5];
		tp.ueset = rq[8]; tp.ueset <<= 8; tp.ueset += rq[7];

		// send ACK
		send_ack();
		break;

	case CMD_RESET:
		if (rq[1] == RESET_HEATER)
		{
			tp.uhset = 0;
			tp.ihset = 0;
		}
		else
		{
			tp.uaset = 0;
			tp.ueset = 0;
			tp.ugset = 240u;
		}

		// send ACK
		send_ack();
		break;

	case CMD_STATUS:
		// send DATA
		send_data(
			tp.a1_a2, 
			(uint16_t[]){
				muhadc,
				mihadc,
				mugadc,
				muaadc,
				miaadc,
				mueadc,
				mieadc,
				mtmadc
			}, 
			8);
		break;

	default:
		// send ERROR
		send_error(VTT_ERR_INVALID_CMD);
		break;
	}
}

int main(void)
{
	configure_processor();
	configure_ports();
	configure_uart();
	sei();

	sample_adc();

	for (;;)
	{
		// on receive command, handle it
		uint8_t* bufin = comm_rx_msg();
		if (bufin)
		{
			handle_rx_msg(bufin);
		}
	}
}
