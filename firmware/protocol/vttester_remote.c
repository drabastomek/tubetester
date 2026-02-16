/*
 * VTTester Protocol v0.2 - Parse 8-byte frame, build ACK and MEAS result.
 * Caller owns RX/TX buffers; no internal state. Integer-only.
 */

#include "vttester_remote.h"
#include "config/config.h"
/* PROGMEM/pgm_read_byte only for avr-gcc firmware build; host tests and ICCAVR use RAM table */
#if !defined(VTTESTER_HOST_TEST) && !defined(ICCAVR)
#include <avr/pgmspace.h>
#endif

#define CMD_SET  0
#define CMD_MEAS 1

#if !defined(VTTESTER_HOST_TEST) && !defined(ICCAVR)
static const uint8_t crc8_table[256] PROGMEM = {
#else
static const uint8_t crc8_table[256] = {
#endif
    0x00,0x07,0x0E,0x09,0x1C,0x1B,0x12,0x15,0x38,0x3F,0x36,0x31,0x24,0x23,0x2A,0x2D,
    0x70,0x77,0x7E,0x79,0x6C,0x6B,0x62,0x65,0x48,0x4F,0x46,0x41,0x54,0x53,0x5A,0x5D,
    0xE0,0xE7,0xEE,0xE9,0xFC,0xFB,0xF2,0xF5,0xD8,0xDF,0xD6,0xD1,0xC4,0xC3,0xCA,0xCD,
    0x90,0x97,0x9E,0x99,0x8C,0x8B,0x82,0x85,0xA8,0xAF,0xA6,0xA1,0xB4,0xB3,0xBA,0xBD,
    0xC7,0xC0,0xC9,0xCE,0xDB,0xDC,0xD5,0xD2,0xFF,0xF8,0xF1,0xF6,0xE3,0xE4,0xED,0xEA,
    0xB7,0xB0,0xB9,0xBE,0xAB,0xAC,0xA5,0xA2,0x8F,0x88,0x81,0x86,0x93,0x94,0x9D,0x9A,
    0x27,0x20,0x29,0x2E,0x3B,0x3C,0x35,0x32,0x1F,0x18,0x11,0x16,0x03,0x04,0x0D,0x0A,
    0x57,0x50,0x59,0x5E,0x4B,0x4C,0x45,0x42,0x6F,0x68,0x61,0x66,0x73,0x74,0x7D,0x7A,
    0x89,0x8E,0x87,0x80,0x95,0x92,0x9B,0x9C,0xB1,0xB6,0xBF,0xB8,0xAD,0xAA,0xA3,0xA4,
    0xF9,0xFE,0xF7,0xF0,0xE5,0xE2,0xEB,0xEC,0xC1,0xC6,0xCF,0xC8,0xDD,0xDA,0xD3,0xD4,
    0x69,0x6E,0x67,0x60,0x75,0x72,0x7B,0x7C,0x51,0x56,0x5F,0x58,0x4D,0x4A,0x43,0x44,
    0x19,0x1E,0x17,0x10,0x05,0x02,0x0B,0x0C,0x21,0x26,0x2F,0x28,0x3D,0x3A,0x33,0x34,
    0x4E,0x49,0x40,0x47,0x52,0x55,0x5C,0x5B,0x76,0x71,0x78,0x7F,0x6A,0x6D,0x64,0x63,
    0x3E,0x39,0x30,0x37,0x22,0x25,0x2C,0x2B,0x06,0x01,0x08,0x0F,0x1A,0x1D,0x14,0x13,
    0xAE,0xA9,0xA0,0xA7,0xB2,0xB5,0xBC,0xBB,0x96,0x91,0x98,0x9F,0x8A,0x8D,0x84,0x83,
    0xDE,0xD9,0xD0,0xD7,0xC2,0xC5,0xCC,0xCB,0xE6,0xE1,0xE8,0xEF,0xFA,0xFD,0xF4,0xF3
};

static uint8_t crc8_compute(const uint8_t *data, uint8_t len)
{
    uint8_t legacy_crc = 0;
#if !defined(VTTESTER_HOST_TEST) && !defined(ICCAVR)
    while (len--) legacy_crc = pgm_read_byte(&crc8_table[legacy_crc ^ *data++]);
#else
    while (len--) legacy_crc = crc8_table[legacy_crc ^ *data++];
#endif
    return legacy_crc;
}

static void frame_append_crc(uint8_t *f)
{
    f[7] = crc8_compute(f, 7);
}

/* HEAT_IDX 0..7 -> voltage_heater_set (0.1V). 0=off, 1=1.4, 2=2.0, 3=2.5, 4=4.0, 5=5.0, 6=6.3, 7=12.6 */
static const uint8_t heat_voltage[8] = { 0, 14, 20, 25, 40, 50, 63, 126 };

/* Simplified SET encoding (no resolution bits): P1 low nibble = heat 0..7; P2/P3 = byte*10 V, max 300 V;
   P4 -> voltage_g1_set = P4*5 (0.1V magnitude, 0..240; 240 = -24 V, same as old code and protocol).
   Byte 6 = heating_time_ticks index 0..63 (500ms/step), tuh_ticks = index*TUH_TICK_SCALE. Main app: setpoint_ug1 = liczug1(parsed.set.voltage_g1_set). */
uint8_t vttester_parse_message(const uint8_t *frame, vttester_parsed_t *out)
{
    uint8_t cmd, p1, p2, p3, p4, tuh_index;
    uint8_t heat_idx;
    uint16_t ua_v, ug2_v, p4_val_01;

    out->cmd = VTTESTER_CMD_NONE;
    out->index = frame[1];
    out->err_code = VTTESTER_ERR_UNKNOWN;
    out->set.error_param = 0;
    out->set.error_value = 0;

    if (crc8_compute(frame, 7) != frame[7]) {
        out->err_code = VTTESTER_ERR_CRC;  /* Frame corrupted; reject so host can retry. */
        return VTTESTER_CMD_NONE;
    }
    cmd = (frame[0] >> 6) & 0x03;
    if (!(frame[0] & 0x20)) {
        out->err_code = VTTESTER_ERR_INVALID_CMD;  /* Command byte bit 5 must be set (valid CMD). */
        return VTTESTER_CMD_NONE;
    }

    if (cmd == CMD_MEAS) {
        out->cmd = VTTESTER_CMD_MEAS;
        return VTTESTER_CMD_MEAS;
    }

    // ERROR HANDLING
    if (cmd != CMD_SET) {
        out->err_code = VTTESTER_ERR_INVALID_CMD;  /* Unknown command (not SET or MEAS). */
        return VTTESTER_CMD_NONE;
    }

    p1 = frame[2]; p2 = frame[3]; p3 = frame[4]; p4 = frame[5]; tuh_index = frame[6];

    heat_idx = p1 & 0x0F;
    if (heat_idx > VTTESTER_SET_HEAT_IDX_MAX) {
        out->cmd = VTTESTER_CMD_SET;
        out->err_code = VTTESTER_ERR_OUT_OF_RANGE;  /* P1: heater index only 0..7. */
        out->set.error_param = 1;
        out->set.error_value = heat_idx;
        return VTTESTER_CMD_SET;
    }
    out->set.voltage_heater_set = heat_voltage[heat_idx];
    out->set.current_heater_set = 0;

    ua_v = (uint16_t)p2 * VTTESTER_SET_UA_SCALE;
    if (ua_v > (uint16_t)VTTESTER_SET_UA_MAX) {
        out->cmd = VTTESTER_CMD_SET;
        out->err_code = VTTESTER_ERR_OUT_OF_RANGE;  /* P2: Ua 10 V steps, max 300 V. */
        out->set.error_param = 2;
        out->set.error_value = ua_v;
        return VTTESTER_CMD_SET;
    }
    out->set.voltage_anode_set = ua_v;

    ug2_v = (uint16_t)p3 * VTTESTER_SET_UA_SCALE;
    if (ug2_v > (uint16_t)VTTESTER_SET_UG2_MAX) {
        out->cmd = VTTESTER_CMD_SET;
        out->err_code = VTTESTER_ERR_OUT_OF_RANGE;  /* P3: Ug2 10 V steps, max 300 V. */
        out->set.error_param = 3;
        out->set.error_value = ug2_v;
        return VTTESTER_CMD_SET;
    }
    out->set.voltage_screen_set = ug2_v;

    p4_val_01 = (uint16_t)p4 * VTTESTER_SET_UG1_P4_STEP;
    if (p4_val_01 > (uint16_t)VTTESTER_SET_UG1_MAX) {
        out->cmd = VTTESTER_CMD_SET;
        out->err_code = VTTESTER_ERR_OUT_OF_RANGE;  /* P4: voltage_g1_set in 0.1V magnitude, max 240 (= -24 V). */
        out->set.error_param = 4;
        out->set.error_value = (uint16_t)p4;
        return VTTESTER_CMD_SET;
    }
    /* Same as old code: voltage_g1_set 0..240 in 0.1V units, 240 = -24.0 V (liczug1(240) in main app). */
    out->set.voltage_g1_set = (uint8_t)p4_val_01;

    if (tuh_index > VTTESTER_SET_TUH_INDEX_MAX) {
        out->cmd = VTTESTER_CMD_SET;
        out->err_code = VTTESTER_ERR_OUT_OF_RANGE;  /* Param 5: heating_time_ticks index 0..63 (500 ms per step). */
        out->set.error_param = 5;
        out->set.error_value = tuh_index;
        return VTTESTER_CMD_SET;
    }
    out->err_code = VTTESTER_ERR_OK;  /* All SET params within ia_range_hi. */
    out->set.tuh_ticks = (uint16_t)tuh_index * VTTESTER_SET_TUH_TICK_SCALE;

    out->cmd = VTTESTER_CMD_SET;
    return VTTESTER_CMD_SET;
}

void vttester_send_response(uint8_t *lcd_line_buffer, uint8_t index, uint8_t err_code,
    uint8_t param_id, uint16_t value)
{
    lcd_line_buffer[0] = 0x02;
    lcd_line_buffer[1] = index;
    lcd_line_buffer[2] = err_code;
    lcd_line_buffer[3] = lcd_line_buffer[4] = lcd_line_buffer[5] = lcd_line_buffer[6] = 0;
    if (err_code == VTTESTER_ERR_OUT_OF_RANGE) {
        lcd_line_buffer[3] = param_id;
        lcd_line_buffer[4] = (uint8_t)(value >> 8);
        lcd_line_buffer[5] = (uint8_t)(value & 0xFF);
    }
    frame_append_crc(lcd_line_buffer);
}

void vttester_send_measurement(uint8_t *lcd_line_buffer, uint8_t index,
    uint16_t lcd_ih, uint16_t lcd_ia, uint8_t ia_range_lcd,
    uint16_t lcd_ig2, uint16_t lcd_s, uint8_t alarm_bits)
{
    uint16_t resistance_r;
    (void)ia_range_lcd;

    lcd_line_buffer[0] = 0x02;
    lcd_line_buffer[1] = index;
    if (lcd_ih > 255) resistance_r = 255; else resistance_r = lcd_ih;
    lcd_line_buffer[2] = (uint8_t)resistance_r;
    resistance_r = lcd_ia + 5;
    resistance_r /= 10;
    if (resistance_r > 255) resistance_r = 255;
    lcd_line_buffer[3] = (uint8_t)resistance_r;
    resistance_r = (uint16_t)lcd_ig2 * 10u + 8u;
    resistance_r /= 16u;
    if (resistance_r > 255) resistance_r = 255;
    lcd_line_buffer[4] = (uint8_t)resistance_r;
    resistance_r = lcd_s + 5;
    resistance_r /= 10;
    if (resistance_r > 255) resistance_r = 255;
    lcd_line_buffer[5] = (uint8_t)resistance_r;
    lcd_line_buffer[6] = alarm_bits & 0xF0;
    frame_append_crc(lcd_line_buffer);
}
