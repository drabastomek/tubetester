/*
 * VTTester Protocol v0.2 - Remote layer implementation
 * CRC-8, RX state machine, frame build (SET ACK, MEAS ACK, MEAS RESULT).
 * Integer-only; no float. Safe to call vttester_feed_byte from ISR.
 */

#include "vttester_remote.h"

#define RX_RING_SIZE 32  /* must be power of 2 */
#define CMD_SET  0
#define CMD_MEAS 1

static unsigned char crc8_table[256] = {
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

static unsigned char crc8_compute(const unsigned char *data, unsigned char len)
{
    unsigned char crc = 0;
    while (len--) crc = crc8_table[crc ^ *data++];
    return crc;
}

static void frame_append_crc(unsigned char *f)
{
    f[7] = crc8_compute(f, 7);
}

/* RX: ring buffer (ISR pushes, poll pulls) */
static unsigned char rx_ring[RX_RING_SIZE];
static unsigned char rx_head;
static unsigned char rx_tail;

/* Current frame being assembled */
static unsigned char rx_frame[VTTESTER_FRAME_LEN];
static unsigned char rx_pos;

/* Last valid frame (for get_last_frame) and event */
static unsigned char last_frame[VTTESTER_FRAME_LEN];
static unsigned char pending_event;

/* TX: single frame queue */
static unsigned char tx_frame[VTTESTER_FRAME_LEN];
static unsigned char tx_pending;

void vttester_init(void)
{
    rx_head = rx_tail = 0;
    rx_pos = 0;
    pending_event = VTTESTER_EVENT_NONE;
    tx_pending = 0;
}

void vttester_feed_byte(unsigned char b)
{
    unsigned char next = (rx_head + 1) & (RX_RING_SIZE - 1);
    if (next != rx_tail) {
        rx_ring[rx_head] = b;
        rx_head = next;
    }
}

unsigned char vttester_poll(void)
{
    unsigned char ev = VTTESTER_EVENT_NONE;
    unsigned char cmd;

    pending_event = VTTESTER_EVENT_NONE;

    while (rx_tail != rx_head) {
        unsigned char b = rx_ring[rx_tail];
        rx_tail = (rx_tail + 1) & (RX_RING_SIZE - 1);

        rx_frame[rx_pos++] = b;
        if (rx_pos >= VTTESTER_FRAME_LEN) {
            rx_pos = 0;
            if (crc8_compute(rx_frame, 7) != rx_frame[7])
                continue; /* CRC error, discard */
            cmd = (rx_frame[0] >> 6) & 0x03;
            if (!(rx_frame[0] & 0x20))
                continue; /* not SETTINGS (REMOTE mode) */
            if (cmd == CMD_SET) {
                ev = VTTESTER_EVENT_SET;
                break;
            }
            if (cmd == CMD_MEAS) {
                ev = VTTESTER_EVENT_MEAS;
                break;
            }
            /* READ or unknown: ignore for now */
        }
    }

    if (ev != VTTESTER_EVENT_NONE) {
        for (cmd = 0; cmd < VTTESTER_FRAME_LEN; cmd++)
            last_frame[cmd] = rx_frame[cmd];
        pending_event = ev;
    }
    return pending_event;
}

void vttester_get_last_frame(unsigned char *out)
{
    unsigned char i;
    for (i = 0; i < VTTESTER_FRAME_LEN; i++)
        out[i] = last_frame[i];
}

void vttester_send_set_ack(unsigned char index, unsigned char error_code)
{
    tx_frame[0] = 0x02;
    tx_frame[1] = index;
    tx_frame[2] = error_code;
    tx_frame[3] = tx_frame[4] = tx_frame[5] = tx_frame[6] = 0;
    frame_append_crc(tx_frame);
    tx_pending = 1;
}

void vttester_send_set_ack_out_of_range(unsigned char index, unsigned char param_id, unsigned int value)
{
    tx_frame[0] = 0x02;
    tx_frame[1] = index;
    tx_frame[2] = VTTESTER_ERR_OUT_OF_RANGE;
    tx_frame[3] = param_id;
    tx_frame[4] = (unsigned char)(value >> 8);
    tx_frame[5] = (unsigned char)(value & 0xFF);
    tx_frame[6] = 0;
    frame_append_crc(tx_frame);
    tx_pending = 1;
}

void vttester_send_meas_ack(unsigned char index)
{
    tx_frame[0] = 0x02;
    tx_frame[1] = index;
    tx_frame[2] = tx_frame[3] = tx_frame[4] = tx_frame[5] = tx_frame[6] = 0;
    frame_append_crc(tx_frame);
    tx_pending = 1;
}

void vttester_send_meas_result(unsigned char index,
    unsigned int ihlcd, unsigned int ialcd, unsigned char rangelcd,
    unsigned int ig2lcd, unsigned int slcd, unsigned char alarm_bits)
{
    unsigned int r;

    tx_frame[0] = 0x02;
    tx_frame[1] = index;
    /* R1: IH_IDX (ihlcd 0..250 = 0..2.5A, 10mA/step) */
    if (ihlcd > 255) r = 255; else r = ihlcd;
    tx_frame[2] = (unsigned char)r;
    /* R2: IA_IDX (ialcd in 0.01mA -> mA) */
    r = ialcd + 5;
    r /= 10;
    if (r > 255) r = 255;
    tx_frame[3] = (unsigned char)r;
    /* R3: IG2_IDX (ig2lcd 0.01mA -> 0.16mA step) */
    r = (unsigned int)ig2lcd * 10u + 8u;
    r /= 16u;
    if (r > 255) r = 255;
    tx_frame[4] = (unsigned char)r;
    /* R4: S_IDX (slcd 0..999 -> 0.1 mA/V) */
    r = slcd + 5;
    r /= 10;
    if (r > 255) r = 255;
    tx_frame[5] = (unsigned char)r;
    tx_frame[6] = alarm_bits & 0xF0; /* R5: alarm bitmap, low nibble reserved */
    frame_append_crc(tx_frame);
    tx_pending = 1;
}

unsigned char vttester_has_pending_tx(void)
{
    return tx_pending;
}

void vttester_get_pending_tx(unsigned char *buf)
{
    unsigned char i;
    for (i = 0; i < VTTESTER_FRAME_LEN; i++)
        buf[i] = tx_frame[i];
}

void vttester_clear_pending_tx(void)
{
    tx_pending = 0;
}
