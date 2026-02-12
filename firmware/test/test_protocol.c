/*
 * Unit tests for VTTester protocol: parse_message, send_response, send_measurement.
 */
#include "test_common.h"
#include "../protocol/vttester_remote.h"
#include <stdio.h>

/* Compute CRC-8 (same polynomial as firmware) for building valid frames. */
static unsigned char crc8(const unsigned char *data, unsigned char len)
{
   static const unsigned char table[256] = {
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
   unsigned char c = 0;
   while (len--) c = table[c ^ *data++];
   return c;
}

static void build_frame(unsigned char *f, unsigned char ctrl, unsigned char idx,
   unsigned char p1, unsigned char p2, unsigned char p3, unsigned char p4, unsigned char p5)
{
   f[0] = ctrl;
   f[1] = idx;
   f[2] = p1;
   f[3] = p2;
   f[4] = p3;
   f[5] = p4;
   f[6] = p5;
   f[7] = crc8(f, 7);
}

/* --- Parse: CRC --- */
static void test_parse_crc_error(void)
{
   unsigned char frame[8];
   vttester_parsed_t parsed;
   build_frame(frame, 0x20, 0x00, 0, 0, 0, 0, 0);
   frame[7] ^= 0x01; /* corrupt CRC */
   unsigned char cmd = vttester_parse_message(frame, &parsed);
   TEST_ASSERT(cmd == VTTESTER_CMD_NONE);
   TEST_ASSERT(parsed.err_code == VTTESTER_ERR_CRC);
}

/* --- Parse: invalid cmd (bit 5 clear) --- */
static void test_parse_invalid_cmd_bit5(void)
{
   unsigned char frame[8];
   vttester_parsed_t parsed;
   /* SET but bit 5 = 0 -> invalid */
   build_frame(frame, 0x00, 0x00, 0, 0, 0, 0, 0);
   unsigned char cmd = vttester_parse_message(frame, &parsed);
   TEST_ASSERT(cmd == VTTESTER_CMD_NONE);
   TEST_ASSERT(parsed.err_code == VTTESTER_ERR_INVALID_CMD);
}

/* --- Parse: MEAS --- */
static void test_parse_meas(void)
{
   unsigned char frame[8];
   vttester_parsed_t parsed;
   build_frame(frame, 0x60, 0x11, 0, 0, 0, 0, 0); /* MEAS, index 0x11 */
   unsigned char cmd = vttester_parse_message(frame, &parsed);
   TEST_ASSERT(cmd == VTTESTER_CMD_MEAS);
   TEST_ASSERT(parsed.index == 0x11);
}

/* --- Parse: SET valid (heat 0..7, Ua/UG2 0..30, Ug1 P4*5<=240, P5 0..63) --- */
static void test_parse_set_ok(void)
{
   unsigned char frame[8];
   vttester_parsed_t parsed;
   /* SET: heat=6 (6.3V), P2=20 (200V), P3=15 (150V), P4=24 (ug1def=120), P5=10 */
   build_frame(frame, 0x20, 0x00, 0x06, 20, 15, 24, 10);
   unsigned char cmd = vttester_parse_message(frame, &parsed);
   TEST_ASSERT(cmd == VTTESTER_CMD_SET);
   TEST_ASSERT(parsed.err_code == VTTESTER_ERR_OK);
   TEST_ASSERT(parsed.set.uhdef == 63);   /* heat_voltage[6] = 6.3V -> 63 in 0.1V (0=off,1=14,2=20,3=25,4=40,5=50,6=63,7=126) */
   TEST_ASSERT(parsed.set.uadef == 200);
   TEST_ASSERT(parsed.set.ug2def == 150);
   TEST_ASSERT(parsed.set.ug1def == 120); /* P4*5 = 120 */
   TEST_ASSERT(parsed.set.tuh_ticks == 20);
}

/* --- Parse: SET P1 out of range (heat index > 7) --- */
static void test_parse_set_p1_oob(void)
{
   unsigned char frame[8];
   vttester_parsed_t parsed;
   build_frame(frame, 0x20, 0x00, 0x08, 0, 0, 0, 0); /* heat idx 8 */
   unsigned char cmd = vttester_parse_message(frame, &parsed);
   TEST_ASSERT(cmd == VTTESTER_CMD_SET);
   TEST_ASSERT(parsed.err_code == VTTESTER_ERR_OUT_OF_RANGE);
   TEST_ASSERT(parsed.set.error_param == 1);
   TEST_ASSERT(parsed.set.error_value == 8);
}

/* --- Parse: SET P2 out of range (Ua > 300 V) --- */
static void test_parse_set_p2_oob(void)
{
   unsigned char frame[8];
   vttester_parsed_t parsed;
   build_frame(frame, 0x20, 0x00, 0, 31, 0, 0, 0); /* 31*10 = 310 */
   unsigned char cmd = vttester_parse_message(frame, &parsed);
   TEST_ASSERT(cmd == VTTESTER_CMD_SET);
   TEST_ASSERT(parsed.err_code == VTTESTER_ERR_OUT_OF_RANGE);
   TEST_ASSERT(parsed.set.error_param == 2);
   TEST_ASSERT(parsed.set.error_value == 310);
}

/* --- Parse: SET P3 out of range (Ug2 > 300 V) --- */
static void test_parse_set_p3_oob(void)
{
   unsigned char frame[8];
   vttester_parsed_t parsed;
   build_frame(frame, 0x20, 0x00, 0, 0, 31, 0, 0);
   unsigned char cmd = vttester_parse_message(frame, &parsed);
   TEST_ASSERT(cmd == VTTESTER_CMD_SET);
   TEST_ASSERT(parsed.err_code == VTTESTER_ERR_OUT_OF_RANGE);
   TEST_ASSERT(parsed.set.error_param == 3);
}

/* --- Parse: SET P4 out of range (ug1def > 240) --- */
static void test_parse_set_p4_oob(void)
{
   unsigned char frame[8];
   vttester_parsed_t parsed;
   build_frame(frame, 0x20, 0x00, 0, 0, 0, 49, 0); /* 49*5 = 245 > 240 */
   unsigned char cmd = vttester_parse_message(frame, &parsed);
   TEST_ASSERT(cmd == VTTESTER_CMD_SET);
   TEST_ASSERT(parsed.err_code == VTTESTER_ERR_OUT_OF_RANGE);
   TEST_ASSERT(parsed.set.error_param == 4);
   TEST_ASSERT(parsed.set.error_value == 49);
}

/* --- Parse: SET P5 out of range (> 63) --- */
static void test_parse_set_p5_oob(void)
{
   unsigned char frame[8];
   vttester_parsed_t parsed;
   build_frame(frame, 0x20, 0x00, 0, 0, 0, 0, 64);
   unsigned char cmd = vttester_parse_message(frame, &parsed);
   TEST_ASSERT(cmd == VTTESTER_CMD_SET);
   TEST_ASSERT(parsed.err_code == VTTESTER_ERR_OUT_OF_RANGE);
   TEST_ASSERT(parsed.set.error_param == 5);
   TEST_ASSERT(parsed.set.error_value == 64);
}

/* --- Parse: SET Ug1 direction (P4=0 -> ug1def=0, P4=48 -> ug1def=240) --- */
static void test_parse_set_ug1_range(void)
{
   unsigned char frame[8];
   vttester_parsed_t parsed;
   build_frame(frame, 0x20, 0x00, 0, 0, 0, 0, 0);
   (void)vttester_parse_message(frame, &parsed);
   TEST_ASSERT(parsed.set.ug1def == 0);
   build_frame(frame, 0x20, 0x00, 0, 0, 0, 48, 0);
   (void)vttester_parse_message(frame, &parsed);
   TEST_ASSERT(parsed.set.ug1def == 240);
}

/* --- send_response: OK vs OUT_OF_RANGE layout --- */
static void test_send_response_ok(void)
{
   unsigned char buf[8];
   vttester_send_response(buf, 0x11, VTTESTER_ERR_OK, 0, 0);
   TEST_ASSERT(buf[0] == 0x02);
   TEST_ASSERT(buf[1] == 0x11);
   TEST_ASSERT(buf[2] == VTTESTER_ERR_OK);
   TEST_ASSERT(buf[3] == 0 && buf[4] == 0 && buf[5] == 0 && buf[6] == 0);
   TEST_ASSERT(buf[7] == crc8(buf, 7));
}

static void test_send_response_out_of_range(void)
{
   unsigned char buf[8];
   vttester_send_response(buf, 0x22, VTTESTER_ERR_OUT_OF_RANGE, 2, 310);
   TEST_ASSERT(buf[0] == 0x02);
   TEST_ASSERT(buf[1] == 0x22);
   TEST_ASSERT(buf[2] == VTTESTER_ERR_OUT_OF_RANGE);
   TEST_ASSERT(buf[3] == 2);
   TEST_ASSERT(buf[4] == 1);   /* 310 >> 8 */
   TEST_ASSERT(buf[5] == 54); /* 310 & 0xFF */
   TEST_ASSERT(buf[6] == 0);
   TEST_ASSERT(buf[7] == crc8(buf, 7));
}

/* --- send_measurement: frame layout and CRC --- */
static void test_send_measurement(void)
{
   unsigned char buf[8];
   vttester_send_measurement(buf, 0x11, 100, 500, 0, 200, 999, 0xF0);
   TEST_ASSERT(buf[0] == 0x02);
   TEST_ASSERT(buf[1] == 0x11);
   TEST_ASSERT(buf[2] == 100);
   /* buf[3] = (500+5)/10 = 50 */
   TEST_ASSERT(buf[3] == 50);
   /* buf[4] = (200*10+8)/16 = 125 */
   TEST_ASSERT(buf[4] == 125);
   /* buf[5] = (999+5)/10 = 100 */
   TEST_ASSERT(buf[5] == 100);
   TEST_ASSERT(buf[6] == 0xF0);
   TEST_ASSERT(buf[7] == crc8(buf, 7));
}

void run_protocol_tests(void)
{
   test_parse_crc_error();
   test_parse_invalid_cmd_bit5();
   test_parse_meas();
   test_parse_set_ok();
   test_parse_set_p1_oob();
   test_parse_set_p2_oob();
   test_parse_set_p3_oob();
   test_parse_set_p4_oob();
   test_parse_set_p5_oob();
   test_parse_set_ug1_range();
   test_send_response_ok();
   test_send_response_out_of_range();
   test_send_measurement();
}
