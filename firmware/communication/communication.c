#include <string.h>

#include "communication.h"
#include "config/config.h"

// variable defines
static const uint8_t CRC8TABLE[256] = {
	0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
	157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220,
	35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98,
	190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
	70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7,
	219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
	101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
	248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185,
	140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205,
	17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
	175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
	50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
	202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139,
	87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
	233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
	116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53
};

static uint8_t in_msg_t[FRAME_RX_BYTES];
static uint8_t out_data_t[FRAME_TX_DATA];
static uint8_t out_ack_t[FRAME_TX_ACK];
static uint8_t out_error_t[FRAME_TX_ERROR];
static uint8_t out_oor_error_t[FRAME_TX_OOR_ERROR];

static volatile uint8_t rx_n;
static uint8_t rx_buf[FRAME_RX_BYTES];
static volatile uint8_t host_ready;


// CRC calculation
uint8_t comm_crc8_run(const uint8_t *data, uint8_t len)
{
	uint8_t c;
	uint8_t i;

	c = 0;
	for (i = 0; i < len; i++)
		c = CRC8TABLE[c ^ data[i]];
	return c;
}

// send ACK
void send_ack(void)
{
	out_ack_t[0] = RSP_ACK;
	out_ack_t[FRAME_TX_ACK - 1u] = comm_crc8_run(out_ack_t, FRAME_TX_ACK - 1u);
	send_message(out_ack_t, FRAME_TX_ACK);
}

// send ERROR
void send_error(uint8_t error_code)
{
	out_error_t[0] = RSP_ERROR;
	out_error_t[1] = error_code;
	out_error_t[FRAME_TX_ERROR - 1u] = comm_crc8_run(out_error_t, FRAME_TX_ERROR - 1u);
	send_message(out_error_t, FRAME_TX_ERROR);
}

// send INPUT RANGE ERROR
void send_input_range_error(uint8_t parameter_id, uint16_t value)
{
	out_oor_error_t[0] = RSP_ERROR;
	out_oor_error_t[1] = VTT_ERR_OUT_OF_RANGE;
	out_oor_error_t[2] = parameter_id;
	out_oor_error_t[3] = LOW(value);
	out_oor_error_t[4] = HIGH(value);
	out_oor_error_t[FRAME_TX_OOR_ERROR - 1u] = comm_crc8_run(out_oor_error_t, FRAME_TX_OOR_ERROR - 1u);
	send_message(out_oor_error_t, FRAME_TX_OOR_ERROR);
}

// send ALARM
void send_alarm(uint8_t alarm_code)
{
	out_error_t[0] = RSP_ALARM;
	out_error_t[1] = alarm_code;
	out_error_t[FRAME_TX_ERROR - 1u] = comm_crc8_run(out_error_t, FRAME_TX_ERROR - 1u);
	send_message(out_error_t, FRAME_TX_ERROR);
}

// send DATA
void send_data(uint8_t a1_a2, uint16_t *data, uint8_t length)
{
	out_data_t[0] = RSP_DATA;
	out_data_t[1] = a1_a2;

	for (uint8_t i = 0; i < length; i++)
	{
		out_data_t[2 * i + 2u] = LOW(data[i]);  // we've loaded the first two bytes already
		out_data_t[2 * i + 3u] = HIGH(data[i]);
	}
	out_data_t[FRAME_TX_DATA - 1u] = comm_crc8_run(out_data_t, FRAME_TX_DATA - 1u);
	send_message(out_data_t, FRAME_TX_DATA);
}

// send USER_BREAK
void send_user_break(void)
{
	// reuse the out_ack_t buffer as it has the same length
	out_ack_t[0] = RSP_USER_BREAK;
	out_ack_t[FRAME_TX_ACK - 1u] = comm_crc8_run(out_ack_t, FRAME_TX_ACK - 1u);
	send_message(out_ack_t, FRAME_TX_ACK);
}

// send message
void send_message(uint8_t *message, uint8_t length)
{
	for (uint8_t i = 0; i < length; i++) {
		comm_transmit_byte(message[i]);
	}
}

// transmit byte
void comm_transmit_byte(uint8_t b)
{
	while (!(UCSRA & (1u << UDRE)))
		;
	UDR = b;
}

// INTERRUPT HANDLER:receive byte
void comm_receive_byte(uint8_t b)
{
	rx_buf[rx_n++] = b;
	if (rx_n < FRAME_RX_BYTES)
		return;

	rx_n = 0;
	memcpy(in_msg_t, rx_buf, FRAME_RX_BYTES);
	host_ready = 1u;
}

// parse command
uint8_t* comm_rx_msg(void)
{
	if (!host_ready)
		return NULL;

	host_ready = 0;

	if (comm_crc8_run(in_msg_t, 9u) != in_msg_t[9]){
		send_error(VTT_ERR_CRC);
		return NULL;
	}

	return in_msg_t;
}
