/*
 * VTTester binary protocol v0.4 — transport (see protocol/VTTester_Protocol_v0.4.txt).
 *
 * comm_rx_byte(b): feed each received byte (e.g. from USART ISR in TTesterLCD32.c).
 * comm_tx_poll(): call from main — handles pending CRC error / completed host frame, drains TX.
 *
 * No ISR macros or critical sections here: RX byte path may run in an ISR; TX queue and
 * handle_request runs only from comm_tx_poll (main). SET → ACK + four stub DATA frames.
 */

#include <string.h>

#include "communication.h"
#include "config/config.h"

/** How many complete frames we can buffer for UART TX before dropping. */
#define OUTGOING_FRAME_QUEUE_DEPTH 16u

/* ------------------------------------------------------------------------- */
/* CRC + frame helpers (v0.4 §2.2) */
/* ------------------------------------------------------------------------- */

/** CRC-8 poly 0x07, init 0 — input is bytes 0..6; return value goes in wire byte 7. */
uint8_t calculate_crc8(const uint8_t *p)
{
	uint8_t c = 0; /* accumulator */
	uint8_t i;
	for (i = 0; i < 7u; i++)
		c = CRC8TABLE[c ^ p[i]]; /* one table step per payload byte */
	return c;
}

/** Write 16-bit value v at bytes[offset] and bytes[offset+1], little-endian (low byte first). */
static void frame_put_le16(uint8_t *bytes, unsigned offset, uint16_t v)
{
	bytes[offset] = (uint8_t)(v & 0xFFu);       /* low byte */
	bytes[offset + 1u] = (uint8_t)((v >> 8) & 0xFFu); /* high byte */
}

/** Fill f->crc from f->ctrl..f->p5 (byte 7 on wire). */
static void finalize_frame_crc(frame_t *f)
{
	f->crc = calculate_crc8((const uint8_t *)f);
}

/* ------------------------------------------------------------------------- */
/* Outgoing queue: responses wait here until comm_tx_poll() sends them on UART */
/* ------------------------------------------------------------------------- */

/*
 * outgoing_frame_queue[]     — ring of frames to transmit.
 * outgoing_queue_write_index — next empty slot to append (enqueue).
 * outgoing_queue_read_index  — next frame to pull for UART (dequeue).
 * outgoing_queued_frame_count — number of frames currently stored.
 */
static frame_t outgoing_frame_queue[OUTGOING_FRAME_QUEUE_DEPTH];
static uint8_t outgoing_queue_write_index;
static uint8_t outgoing_queue_read_index;
static uint8_t outgoing_queued_frame_count;

/** Copy one frame into the TX ring; drop silently if ring is full. */
static void enqueue_outgoing_frame(const frame_t *src)
{
	if (outgoing_queued_frame_count >= OUTGOING_FRAME_QUEUE_DEPTH)
		return; /* no room */
	outgoing_frame_queue[outgoing_queue_write_index] = *src;
	outgoing_queue_write_index =
	    (uint8_t)((outgoing_queue_write_index + 1u) % OUTGOING_FRAME_QUEUE_DEPTH);
	outgoing_queued_frame_count++;
}

/* ------------------------------------------------------------------------- */
/* Incoming path: bytes from host assembled into one frame (comm_rx_byte) */
/* ------------------------------------------------------------------------- */

static volatile uint8_t incoming_byte_count; /* 0..8 bytes collected so far */
static uint8_t incoming_raw_bytes[8];        /* raw wire bytes for current frame */

/*
 * crc_error_from_host_flag — ISR saw bad CRC; comm_tx_poll sends ERROR(CRC) then clears.
 * host_request_ready       — one good frame copied to staged_host_request; main handles it.
 * staged_host_request      — last valid 8-byte request from PC (for handle_request).
 */
static volatile uint8_t crc_error_from_host_flag;
static volatile uint8_t host_request_ready;
static frame_t staged_host_request;

/** Draft “measurement in progress” flag (BUSY vs ACK on STATUS). */
static volatile uint8_t vt_meas_busy;

/* ------------------------------------------------------------------------- */
/* Public init + low-level UART transmit (blocking on UDRE) */
/* ------------------------------------------------------------------------- */

void comm_init(void)
{
	incoming_byte_count = 0;
	crc_error_from_host_flag = 0;
	host_request_ready = 0;
	vt_meas_busy = 0;
	outgoing_queue_write_index = 0;
	outgoing_queue_read_index = 0;
	outgoing_queued_frame_count = 0;
}

/** Send one byte on USART; spin until transmit buffer empty. */
void char2rs(uint8_t bajt)
{
	while (!(UCSRA & (1 << UDRE)))
		; /* wait: data register can accept new byte */
	UDR = bajt;
}

/** Send a C string (for debug ASCII only). */
void cstr2rs(const char *q)
{
	while (*q)
	{
		while (!(UCSRA & (1 << UDRE)))
			;
		UDR = *q++;
	}
}

/* ------------------------------------------------------------------------- */
/* Build standard ERROR / BUSY / ACK frames and queue them */
/* ------------------------------------------------------------------------- */

/** ERROR: R1=CRC_ERROR (v0.4 §11); INDEX=0; rest zero. */
static void reply_error_crc(void)
{
	frame_t f;
	f.ctrl = VTT_RSP_ERROR;
	f.idx = 0;
	f.p1 = VTT_ERR_CRC; /* error code in byte 2 (R1) */
	f.p2 = f.p3 = f.p4 = f.p5 = 0;
	finalize_frame_crc(&f);
	enqueue_outgoing_frame(&f);
}

/** ERROR: invalid command / reserved CTRL bits (v0.4 §3–4). */
static void reply_error_invalid(uint8_t idx)
{
	frame_t f;
	f.ctrl = VTT_RSP_ERROR;
	f.idx = idx; /* echo request INDEX */
	f.p1 = VTT_ERR_INVALID_CMD;
	f.p2 = f.p3 = f.p4 = f.p5 = 0;
	finalize_frame_crc(&f);
	enqueue_outgoing_frame(&f);
}

/** ERROR: SET parameter out of draft range; R2=param id, R3–R4 = 16-bit attempted (big-endian in p3,p4). */
static void reply_error_range(uint8_t idx, uint8_t param_id, uint16_t attempted)
{
	frame_t f;
	f.ctrl = VTT_RSP_ERROR;
	f.idx = idx;
	f.p1 = VTT_ERR_OUT_OF_RANGE;
	f.p2 = param_id;
	f.p3 = (uint8_t)((attempted >> 8) & 0xFFu); /* high byte of attempted value */
	f.p4 = (uint8_t)(attempted & 0xFFu);        /* low byte */
	f.p5 = 0;
	finalize_frame_crc(&f);
	enqueue_outgoing_frame(&f);
}

/** BUSY: measurement already running (draft). */
static void reply_busy(uint8_t idx)
{
	frame_t f;
	f.ctrl = VTT_RSP_BUSY;
	f.idx = idx;
	f.p1 = f.p2 = f.p3 = f.p4 = f.p5 = 0;
	finalize_frame_crc(&f);
	enqueue_outgoing_frame(&f);
}

/** ACK: command accepted; payload bytes zero for simple ack. */
static void reply_ack(uint8_t idx)
{
	frame_t f;
	f.ctrl = VTT_RSP_ACK;
	f.idx = idx;
	f.p1 = f.p2 = f.p3 = f.p4 = f.p5 = 0;
	finalize_frame_crc(&f);
	enqueue_outgoing_frame(&f);
}

/* ------------------------------------------------------------------------- */
/* Stub: four DATA frames after SET (v0.4 §10.2) — fake 64-sample ADC sums */
/* ------------------------------------------------------------------------- */

/**
 * Queue four DATA frames with constant demo values (replace with real m*adc sums).
 * Per v0.4 §10.1, uint16 fields are sums of 64 measurements; PC converts to SI units.
 * idx = echo tube/system INDEX in each frame.
 */
static void enqueue_data_stub(uint8_t idx)
{
	frame_t f;
	const uint16_t uh = 40320u;  /* demo sum (64× typical raw bucket) */
	const uint16_t ih = 28160u;
	const uint16_t ua = 51200u;
	const uint16_t ug2 = 48000u;
	const uint16_t ia = 9600u;
	const uint16_t ig2 = 640u;
	const uint16_t ug1 = 8000u; /* still a raw sum in real firmware; demo only */
	const uint8_t alarm = 0;    /* bits 7..4 per §12 */
	const uint8_t dctrl = (uint8_t)(VTT_RSP_DATA | 0x02u); /* DATA + REMOTE in STATE */
	uint8_t *b;

	/* Frame 1: Uh, Ih */
	f.ctrl = dctrl;
	f.idx = idx;
	b = (uint8_t *)&f;
	frame_put_le16(b, 2, uh);  /* p1,p2 = Uh le */
	frame_put_le16(b, 4, ih);  /* p3,p4 = Ih le */
	f.p5 = 0;
	finalize_frame_crc(&f);
	enqueue_outgoing_frame(&f);

	/* Frame 2: Ua, Ug2 */
	f.ctrl = dctrl;
	f.idx = idx;
	b = (uint8_t *)&f;
	frame_put_le16(b, 2, ua);
	frame_put_le16(b, 4, ug2);
	f.p5 = 0;
	finalize_frame_crc(&f);
	enqueue_outgoing_frame(&f);

	/* Frame 3: Ia, Ig2 */
	f.ctrl = dctrl;
	f.idx = idx;
	b = (uint8_t *)&f;
	frame_put_le16(b, 2, ia);
	frame_put_le16(b, 4, ig2);
	f.p5 = 0;
	finalize_frame_crc(&f);
	enqueue_outgoing_frame(&f);

	/* Frame 4: Ug1 magnitude, alarm byte, zeros */
	f.ctrl = dctrl;
	f.idx = idx;
	b = (uint8_t *)&f;
	frame_put_le16(b, 2, ug1);
	f.p3 = alarm; /* protocol: alarm in byte 5 of frame */
	f.p4 = f.p5 = 0;
	finalize_frame_crc(&f);
	enqueue_outgoing_frame(&f);
}

/* ------------------------------------------------------------------------- */
/* SET parameter limits (v0.4 §6): P1 heater value; P2..P5 as before (draft caps) */
/* ------------------------------------------------------------------------- */

/** v0.4 §5: TUBE_TYPE 0-5; SYSTEM in bits 3-1 must be 0, 1, or 2. */
static int index_ok(uint8_t idx)
{
	uint8_t tube = (uint8_t)((idx >> 4) & 0x0Fu);
	uint8_t sys = (uint8_t)((idx >> 1) & 0x07u);

	if (tube > 5u)
		return 0;
	if (sys > 2u)
		return 0;
	return 1;
}

/** Return 1 if SET params OK; else send error frame and return 0. */
static int set_params_ok(const frame_t *rq)
{
	if (!index_ok(rq->idx))
	{
		reply_error_invalid(rq->idx);
		return 0;
	}
	/* P1: v0.4 §6 heater value (0.1 V or 10 mA); single byte 0..255 on wire */
	uint16_t p2 = rq->p2; /* Ua steps (draft max 30) */
	uint16_t p3 = rq->p3; /* Ug2 steps */
	uint16_t p4 = rq->p4; /* Ug1 steps (draft max 240) */
	uint16_t p5 = rq->p5; /* time index (draft max 63) */
	if (p2 > 30u)
	{
		reply_error_range(rq->idx, 2u, p2);
		return 0;
	}
	if (p3 > 30u)
	{
		reply_error_range(rq->idx, 3u, p3);
		return 0;
	}
	if (p4 > 240u)
	{
		reply_error_range(rq->idx, 4u, p4);
		return 0;
	}
	if (p5 > 63u)
	{
		reply_error_range(rq->idx, 5u, p5);
		return 0;
	}
	return 1;
}

/* ------------------------------------------------------------------------- */
/* Dispatch one validated request from PC */
/* ------------------------------------------------------------------------- */

static void handle_request(const frame_t *rq)
{
	uint8_t cmd = (uint8_t)(rq->ctrl & 0xC0u); /* top two bits: SET / STATUS / RESET */

	/* v0.4: low 6 bits of request CTRL must be zero */
	if ((rq->ctrl & 0x3Fu) != 0u)
	{
		reply_error_invalid(rq->idx);
		return;
	}

	switch (cmd)
	{
	case VTT_REQ_SET:
		if (vt_meas_busy)
		{
			reply_busy(rq->idx);
			return;
		}
		if (!set_params_ok(rq))
			return; /* ERROR already queued */
		vt_meas_busy = 1;
		reply_ack(rq->idx);     /* ACK then stub “measurement” data */
		enqueue_data_stub(rq->idx);
		vt_meas_busy = 0;
		break;

	case VTT_REQ_STATUS:
	{
		frame_t f;
		f.ctrl = vt_meas_busy ? VTT_RSP_BUSY : VTT_RSP_ACK;
		f.idx = rq->idx;
		f.p1 = f.p2 = f.p3 = f.p4 = f.p5 = 0;
		finalize_frame_crc(&f);
		enqueue_outgoing_frame(&f);
		break;
	}

	case VTT_REQ_RESET:
	{
		uint8_t p1 = rq->p1; /* RESET scope: Ua/Ug only vs full (§8) */
		if (p1 != VTT_RESET_UA_UG2_UG1 && p1 != VTT_RESET_FULL && p1 != 0u)
			p1 = VTT_RESET_FULL; /* invalid → treat as full (draft) */
		(void)p1; /* hardware power-down not implemented in comms harness */
		vt_meas_busy = 0;
		reply_ack(rq->idx);
		break;
	}

	default:
		reply_error_invalid(rq->idx);
		break;
	}
}

/* ------------------------------------------------------------------------- */
/* RX: called once per received byte (often from USART ISR) */
/* ------------------------------------------------------------------------- */

void comm_rx_byte(uint8_t b)
{
	if (incoming_byte_count < 8u)
		incoming_raw_bytes[incoming_byte_count++] = b; /* append */
	else
	{
		/* overflow: resync — start new frame with this byte */
		incoming_raw_bytes[0] = b;
		incoming_byte_count = 1u;
	}

	if (incoming_byte_count != 8u)
		return; /* wait for full frame */

	/* Compare CRC of bytes 0..6 to received byte 7 */
	if (calculate_crc8(incoming_raw_bytes) != incoming_raw_bytes[7])
	{
		crc_error_from_host_flag = 1u; /* comm_tx_poll will emit ERROR CRC */
		incoming_byte_count = 0;
		return;
	}

	/* One slot: if previous command not yet consumed by main, drop this frame */
	if (!host_request_ready)
	{
		memcpy(&staged_host_request, incoming_raw_bytes, sizeof(frame_t));
		host_request_ready = 1u;
	}
	incoming_byte_count = 0;
}

/* ------------------------------------------------------------------------- */
/* Main loop: service flags, run protocol, send all queued frames */
/* ------------------------------------------------------------------------- */

void comm_tx_poll(void)
{
	frame_t out;
	uint8_t i;

	/* Late CRC error from RX path (must not enqueue from ISR) */
	if (crc_error_from_host_flag)
	{
		crc_error_from_host_flag = 0u;
		reply_error_crc();
	}

	/* One pending good request → protocol handler (may enqueue many replies) */
	if (host_request_ready)
	{
		out = staged_host_request;
		host_request_ready = 0u;
		handle_request(&out);
	}

	/* Drain TX ring: each frame = 8 bytes on UART */
	while (outgoing_queued_frame_count > 0u)
	{
		const uint8_t *bytes;

		out = outgoing_frame_queue[outgoing_queue_read_index];
		outgoing_queue_read_index =
		    (uint8_t)((outgoing_queue_read_index + 1u) % OUTGOING_FRAME_QUEUE_DEPTH);
		outgoing_queued_frame_count--;

		bytes = (const uint8_t *)&out;
		for (i = 0; i < 8u; i++)
			char2rs(bytes[i]);
	}
}
