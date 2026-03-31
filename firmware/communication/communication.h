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

/* INDEX (v0.4 §5): tube (7-4), system (3-1), heater parallel=0 / series=1 (bit 0) */
#define VTT_IDX_HEATER_SERIES 0x01u
#define VTT_IDX_SYSTEM_MASK   0x0Eu

/** One on-wire v0.4 frame (8 bytes); field order matches protocol byte 0..7. */
typedef struct
{
	uint8_t ctrl;
	uint8_t idx;
	uint8_t p1;
	uint8_t p2;
	uint8_t p3;
	uint8_t p4;
	uint8_t p5;
	uint8_t crc;
} frame_t;



/* Table: CRC-8 poly 0x07, init 0 (v0.4 §2.2) */

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

/** CRC-8 poly 0x07 over bytes 0..6; compare to byte 7 of frame. */
uint8_t vtt_crc8(const uint8_t *frame07);

void comm_init(void);

/** Feed one RX byte (e.g. from USART ISR); do not call comm_tx_poll from ISR. */
void comm_rx_byte(uint8_t b);

/** Main loop: process staged host request + drain outgoing frames on UART (TX). */
void comm_tx_poll(void);

extern void char2rs(uint8_t bajt);
extern void cstr2rs(const char *q);

#endif
