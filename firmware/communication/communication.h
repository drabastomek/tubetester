#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stdint.h>

/* PC → firmware (request CTRL, bits 7-6) */
#define VTT_REQ_SET    0x00u
#define VTT_REQ_STATUS 0x40u
#define VTT_REQ_RESET  0x80u

/* Firmware → PC (response CTRL) */
#define VTT_RSP_ACK    0x02u /* ACK + REMOTE in STATE (draft default) */
#define VTT_RSP_BUSY   0x42u
#define VTT_RSP_ERROR  0x82u
#define VTT_RSP_DATA   0x20u /* DATA=1, STATUS=ACK; OR with STATE nibble */

#define VTT_ERR_UNKNOWN      0x00u
#define VTT_ERR_OUT_OF_RANGE 0x02u
#define VTT_ERR_CRC          0x03u
#define VTT_ERR_INVALID_CMD  0x04u
#define VTT_ERR_HARDWARE     0x06u

#define VTT_RESET_UA_UG2_UG1 0x01u
#define VTT_RESET_FULL       0x02u

/* INDEX (v0.4.1 §5): tube (7-4), system (3-1), heater parallel=0 / series=1 (bit 0) */
#define VTT_IDX_HEATER_SERIES 0x01u
#define VTT_IDX_SYSTEM_MASK   0x0Eu

/** Wire frame size (protocol v0.4.1 §2). */
#define VTT_FRAME_BYTES 9u
#define VTT_UA_UG2_MAX 0x0FFFu

/** One on-wire v0.4.1 frame (9 bytes): CTRL, INDEX, P1..P6, CRC (byte 8). */
typedef struct
{
	uint8_t ctrl;
	uint8_t idx;
	uint8_t p1;
	uint8_t p2;
	uint8_t p3;
	uint8_t p4;
	uint8_t p5;
	uint8_t p6;
	uint8_t crc;
} frame_t;

/* Table: CRC-8 poly 0x07, init 0 (v0.4.1 §2.2) */

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

/** CRC-8 poly 0x07 over bytes 0..7; compare to byte 8 of frame. */
uint8_t comm_checksum(const uint8_t *frame08);

void comm_init(void);

/** Feed one RX byte (e.g. from USART ISR); do not call comm_handle_requests from ISR. */
void comm_receive_byte(uint8_t b);

/**
 * Main loop: host CRC error reply, then handle one staged RX frame (handle_request),
 * then drain the outgoing queue on UART (TX).
 */
void comm_handle_requests(void);

/**
 * Link layer: blocking TX of one byte. Not implemented in communication.c — supply
 * in your port file (GCC-AVR: avr/io.h + USART regs; ICCAVR: your register API).
 * Used by comm_handle_requests and by comm_transmit_string.
 */
void comm_transmit_byte(uint8_t b);

/** Optional helper; calls comm_transmit_byte (portable C in communication.c). */
void comm_transmit_string(const char *s);

/**
 * Outbound protocol responses (v0.4.1 §4, §6): enqueue one frame on the TX queue.
 *
 * Call only from normal thread context (e.g. main loop), same rules as
 * comm_handle_requests — not from ISR. The module already uses these internally;
 * exposing them is for application-level errors, tests, or bridges that must
 * signal BUSY/ACK/ERROR without going through a full SET/STATUS/RESET parse.
 *
 * Do not combine carelessly with comm_handle_requests in one iteration in a way
 * that double-ACKs or contradicts vt_meas_busy / measurement_data_pending state.
 */
void reply_error_crc(void);
/** R1 = VTT_ERR_INVALID_CMD; echo idx from the request being rejected. */
void reply_error_invalid(uint8_t idx);
/**
 * R1 = VTT_ERR_OUT_OF_RANGE, R2 = param_id (1..6 per §6), R3–R4 = attempted
 * value big-endian (high, low).
 */
void reply_error_range(uint8_t idx, uint8_t param_id, uint16_t attempted);
/** CTRL = VTT_RSP_BUSY; use when measurement already in progress (§6). */
void reply_busy(uint8_t idx);
/** CTRL = VTT_RSP_ACK, payload zero; success accept (§6 SET response, etc.). */
void reply_ack(uint8_t idx);

/**
 * After a valid SET, firmware ACKs immediately; DATA (3 frames, §10.2) is queued
 * separately so the host sees ACK before RESULT (e.g. heating / settling).
 *
 * comm_measurement_data_pending(): non-zero if a SET is waiting for DATA to be enqueued.
 * comm_get_last_remote_set(): decoded fields from the last accepted SET (P5 = |Ug1| wire units).
 * comm_enqueue_measurement_data(): queue Uh/Ih, Ua/Ug2/Ug1, Ia/Ig2+alarm as uint16 LE sums.
 * comm_clear_measurement_data_pending(): call after enqueue (or to cancel); clears BUSY for STATUS.
 */
uint8_t comm_measurement_data_pending(void);

void comm_get_last_remote_set(
	uint8_t *idx,
	uint8_t *p1_heater,
	uint16_t *ua_12,
	uint16_t *ug2_12,
	uint8_t *p5_ug1_mag,
	uint8_t *p6_time_idx);
void comm_enqueue_measurement_data(
	uint8_t idx,
	uint16_t uh_sum,
	uint16_t ih_sum,
	uint16_t ua_sum,
	uint16_t ug2_sum,
	uint16_t ug1_sum,
	uint16_t ia_sum,
	uint16_t ig2_sum,
	uint8_t alarm);
void comm_clear_measurement_data_pending(void);

#endif
