/*
 * VTTester Protocol v0.2 - Remote (PC <-> FW) binary protocol
 *
 * Frame: 8 bytes. CMD in CTRL (SET=0, MEAS=1, READ=2). CRC-8 on bytes 0..6.
 * Flow: SET -> ACK; MEAS -> ACK (immediate) -> RESULT (when measurement done).
 */

#ifndef VTTESTER_REMOTE_H
#define VTTESTER_REMOTE_H

#define VTTESTER_FRAME_LEN 8

/* --- Error codes (ACK R1 for SET, or when STATUS=ERROR) --- */
#define VTTESTER_ERR_OK              0x00
#define VTTESTER_ERR_UNKNOWN         0x01
#define VTTESTER_ERR_OUT_OF_RANGE   0x02
#define VTTESTER_ERR_CRC            0x03
#define VTTESTER_ERR_INVALID_CMD    0x04
#define VTTESTER_ERR_HARDWARE       0x06

/* --- Alarm bitmap (RESULT R5, and R1 when STATUS=ALARM) --- */
#define VTTESTER_ALARM_OVERIH  0x10  /* Heater overcurrent */
#define VTTESTER_ALARM_OVERIA  0x20  /* Anode overcurrent */
#define VTTESTER_ALARM_OVERIG  0x40  /* Grid 2 overcurrent */
#define VTTESTER_ALARM_OVERTE  0x80  /* Overtemp */

/* --- Events returned by vttester_poll --- */
#define VTTESTER_EVENT_NONE    0
#define VTTESTER_EVENT_SET     1
#define VTTESTER_EVENT_MEAS    2

/* --- API --- */

/* Call once at startup. */
void vttester_init(void);

/* Feed one byte from UART RX (call from ISR or after reading UDR in main). */
void vttester_feed_byte(unsigned char b);

/* Process received bytes; returns VTTESTER_EVENT_* when a valid frame is ready. */
unsigned char vttester_poll(void);

/* Copy last received frame (8 bytes). Call after vttester_poll() returns SET or MEAS. */
void vttester_get_last_frame(unsigned char *out);

/* Request to send SET ACK. error_code = VTTESTER_ERR_* (e.g. OUT_OF_RANGE). */
void vttester_send_set_ack(unsigned char index, unsigned char error_code);

/* SET ACK with OUT_OF_RANGE details: R2=param_id (1..5), R3-R4=16-bit value (high byte R3). */
void vttester_send_set_ack_out_of_range(unsigned char index, unsigned char param_id, unsigned int value);

/* Request to send MEAS immediate ACK ("OK, starting measurement"). */
void vttester_send_meas_ack(unsigned char index);

/* Build and queue MEAS result frame. Uses firmware units: ihlcd (0..250 = 0..2.5A),
   ialcd (0.01 mA, range 0=40mA max, 1=200mA max), ig2lcd (0.01 mA), slcd (0..999 = 0..99.9),
   alarm_bits = VTTESTER_ALARM_* mask. */
void vttester_send_meas_result(unsigned char index,
   unsigned int ihlcd, unsigned int ialcd, unsigned char rangelcd,
   unsigned int ig2lcd, unsigned int slcd, unsigned char alarm_bits);

/* True if a frame is queued to send. Main should send it byte-by-byte then call clear. */
unsigned char vttester_has_pending_tx(void);
void vttester_get_pending_tx(unsigned char *buf);
void vttester_clear_pending_tx(void);

#endif /* VTTESTER_REMOTE_H */
