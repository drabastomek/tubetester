/*
 * VTTester Protocol v0.2 - Remote (PC <-> FW) binary protocol
 *
 * Frame: 8 bytes. CMD in CTRL (SET=0, MEAS=1, READ=2). CRC-8 on bytes 0..6.
 * Caller assembles RX frame and passes to vttester_parse_message; caller
 * provides TX buffer to vttester_send_response / vttester_send_measurement.
 */

#ifndef VTTESTER_REMOTE_H
#define VTTESTER_REMOTE_H

#include <stdint.h>

#define VTTESTER_FRAME_LEN 8

/* --- Parse result: command type --- */
#define VTTESTER_CMD_NONE  0
#define VTTESTER_CMD_SET   1
#define VTTESTER_CMD_MEAS  2

/* --- Error codes (ACK R1 for SET) --- */
#define VTTESTER_ERR_OK              0x00
#define VTTESTER_ERR_UNKNOWN         0x01
#define VTTESTER_ERR_OUT_OF_RANGE    0x02
#define VTTESTER_ERR_CRC             0x03
#define VTTESTER_ERR_INVALID_CMD     0x04
#define VTTESTER_ERR_HARDWARE        0x06

/* --- Alarm bitmap (RESULT R5) --- */
#define VTTESTER_ALARM_OVERIH  0x10
#define VTTESTER_ALARM_OVERIA  0x20
#define VTTESTER_ALARM_OVERIG  0x40
#define VTTESTER_ALARM_OVERTE  0x80

/* --- Parsed SET parameters (from vttester_parse_message when cmd == SET) --- */
typedef struct {
   uint8_t voltage_heater_set;
   uint8_t current_heater_set;
   uint8_t voltage_g1_set;
   uint16_t  voltage_anode_set;
   uint16_t  voltage_screen_set;
   uint16_t  tuh_ticks;
   uint8_t error_param;
   uint16_t  error_value;
} vttester_set_params_t;

/* --- Parsed message (filled by vttester_parse_message) --- */
typedef struct {
   uint8_t cmd;           /* VTTESTER_CMD_* */
   uint8_t index;         /* INDEX byte */
   uint8_t err_code;     /* VTTESTER_ERR_* to send in ACK (parser sets for NONE and SET) */
   vttester_set_params_t set;  /* valid when cmd == SET */
} vttester_parsed_t;

/* --- Public API --- */

/* Parse 8-byte frame. Returns VTTESTER_CMD_NONE, VTTESTER_CMD_SET, or VTTESTER_CMD_MEAS.
   Fills out->err_code (always): for NONE = CRC/INVALID_CMD/UNKNOWN, for SET = OK or OUT_OF_RANGE.
   For SET, out->set.error_param/error_value are set when err_code == OUT_OF_RANGE. */
uint8_t vttester_parse_message(const uint8_t *frame, vttester_parsed_t *out);

/* Build SET or MEAS ACK into lcd_line_buffer (8 bytes). err_code = VTTESTER_ERR_*.
   When err_code == VTTESTER_ERR_OUT_OF_RANGE, param_id (1..5) and value
   are written to R2 and R3-R4; otherwise param_id/value are ignored. */
void vttester_send_response(uint8_t *lcd_line_buffer, uint8_t index, uint8_t err_code,
   uint8_t param_id, uint16_t value);

/* Build MEAS result frame into lcd_line_buffer (8 bytes). lcd_ih 0..250, lcd_ia in 0.01mA,
   ia_range_lcd 0/1, lcd_ig2 0.01mA, lcd_s 0..999, alarm_bits = VTTESTER_ALARM_* mask. */
void vttester_send_measurement(uint8_t *lcd_line_buffer, uint8_t index,
   uint16_t lcd_ih, uint16_t lcd_ia, uint8_t ia_range_lcd,
   uint16_t lcd_ig2, uint16_t lcd_s, uint8_t alarm_bits);

#endif /* VTTESTER_REMOTE_H */
