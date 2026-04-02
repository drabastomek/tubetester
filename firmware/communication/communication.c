/*
 * VTTester binary protocol v0.4.1 — transport (see protocol/VTTester_Protocol_v0.4.1.txt).
 *
 * comm_receive_byte(b): feed each RX byte (e.g. from USART ISR).
 * comm_handle_requests(): call from main — CRC error reply, staged RX → handle_request, drain TX.
 *
 * 9-byte frames: CRC over bytes 0–7, byte 8 = CRC. SET → ACK; DATA triple from main.
 */

#include <string.h>

#include "communication.h"

#define OUTGOING_FRAME_QUEUE_DEPTH 16u

/* ------------------------------------------------------------------------- */
/* CRC + frame helpers (v0.4.1 §2.2) */
/* ------------------------------------------------------------------------- */

uint8_t comm_checksum(const uint8_t *p)
{
	uint8_t c = 0;
	uint8_t i;

	for (i = 0; i < 8u; i++)
		c = CRC8TABLE[c ^ p[i]];
	return c;
}

static void finalize_frame_crc(frame_t *f)
{
	f->crc = comm_checksum((const uint8_t *)f);
}

static uint8_t le16_lo(uint16_t v)
{
	return (uint8_t)(v & 0xFFu);
}

static uint8_t le16_hi(uint16_t v)
{
	return (uint8_t)((v >> 8) & 0xFFu);
}

/* ------------------------------------------------------------------------- */
/* Outgoing queue */
/* ------------------------------------------------------------------------- */

static frame_t outgoing_frame_queue[OUTGOING_FRAME_QUEUE_DEPTH];
static uint8_t outgoing_queue_write_index;
static uint8_t outgoing_queue_read_index;
static uint8_t outgoing_queued_frame_count;

static void enqueue_outgoing_frame(const frame_t *src)
{
	if (outgoing_queued_frame_count >= OUTGOING_FRAME_QUEUE_DEPTH)
		return;
	outgoing_frame_queue[outgoing_queue_write_index] = *src;
	outgoing_queue_write_index =
	    (uint8_t)((outgoing_queue_write_index + 1u) % OUTGOING_FRAME_QUEUE_DEPTH);
	outgoing_queued_frame_count++;
}

/** CTRL, INDEX, P1–P6 set; append CRC (byte 8) and queue for TX. */
static void enqueue_reply_frame(frame_t *f)
{
	finalize_frame_crc(f);
	enqueue_outgoing_frame(f);
}

/* ------------------------------------------------------------------------- */
/* Incoming path */
/* ------------------------------------------------------------------------- */

static volatile uint8_t incoming_byte_count;
static uint8_t incoming_raw_bytes[VTT_FRAME_BYTES];

static volatile uint8_t crc_error_from_host_flag;
static volatile uint8_t host_request_ready;
static frame_t staged_host_request;

static volatile uint8_t vt_meas_busy;

/* Last accepted SET (decoded); DATA frames are enqueued later from main (see §9 flow). */
static uint8_t last_set_idx;
static uint8_t last_set_p1_heater;
static uint16_t last_set_ua_12;
static uint16_t last_set_ug2_12;
static uint8_t last_set_p5_ug1;
static uint8_t last_set_p6_time;
static uint8_t measurement_data_pending;

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
	measurement_data_pending = 0;
}

void comm_transmit_string(const char *s)
{
	if (!s)
		return;
	while (*s)
		comm_transmit_byte((uint8_t)*s++);
}

/* ------------------------------------------------------------------------- */

static void frame_zero_payload(frame_t *f)
{
	f->p1 = f->p2 = f->p3 = f->p4 = f->p5 = f->p6 = 0;
}

/**
 * Non-ERROR responses (v0.4.1). Maps rsptype → CTRL; sets INDEX and R1–R6 (P1–P6).
 * ACK/BUSY: pass 0 for p1..p6. DATA (§10.2): pass uint16 LE as (lo,hi) pairs per field.
 */
static void reply_to_pc(
	uint8_t rsptype,
	uint8_t idx,
	uint8_t p1,
	uint8_t p2,
	uint8_t p3,
	uint8_t p4,
	uint8_t p5,
	uint8_t p6)
{
	frame_t f;

	f.idx = idx;

	switch (rsptype)
	{
	case VTT_RSP_ACK:
		f.ctrl = VTT_RSP_ACK;
		frame_zero_payload(&f);
		break;
	case VTT_RSP_BUSY:
		f.ctrl = VTT_RSP_BUSY;
		frame_zero_payload(&f);
		break;
	case VTT_RSP_DATA:
		f.ctrl = VTT_RSP_DATA;
		f.p1 = p1;
		f.p2 = p2;
		f.p3 = p3;
		f.p4 = p4;
		f.p5 = p5;
		f.p6 = p6;
		break;
	default:
		f.ctrl = VTT_RSP_ACK;
		frame_zero_payload(&f);
		break;
	}

	enqueue_reply_frame(&f);
}

/** errtype: VTT_ERR_* ; param_id / attempted used for VTT_ERR_OUT_OF_RANGE (R2, R3–R4). */
static void reply_error(uint8_t errtype, uint8_t idx, uint8_t param_id, uint16_t attempted)
{
	frame_t f;

	f.ctrl = VTT_RSP_ERROR;
	f.idx = idx;
	frame_zero_payload(&f);

	switch (errtype)
	{
	case VTT_ERR_CRC:
		f.p1 = VTT_ERR_CRC;
		break;
	case VTT_ERR_INVALID_CMD:
		f.p1 = VTT_ERR_INVALID_CMD;
		break;
	case VTT_ERR_OUT_OF_RANGE:
		f.p1 = VTT_ERR_OUT_OF_RANGE;
		f.p2 = param_id;
		f.p3 = (uint8_t)((attempted >> 8) & 0xFFu);
		f.p4 = (uint8_t)(attempted & 0xFFu);
		break;
	case VTT_ERR_HARDWARE:
		f.p1 = VTT_ERR_HARDWARE;
		break;
	default:
		f.p1 = VTT_ERR_UNKNOWN;
		break;
	}

	enqueue_reply_frame(&f);
}

void reply_error_crc(void)
{
	reply_error(VTT_ERR_CRC, 0u, 0u, 0u);
}

void reply_error_invalid(uint8_t idx)
{
	reply_error(VTT_ERR_INVALID_CMD, idx, 0u, 0u);
}

void reply_error_range(uint8_t idx, uint8_t param_id, uint16_t attempted)
{
	reply_error(VTT_ERR_OUT_OF_RANGE, idx, param_id, attempted);
}

void reply_busy(uint8_t idx)
{
	reply_to_pc(VTT_RSP_BUSY, idx, 0u, 0u, 0u, 0u, 0u, 0u);
}

void reply_ack(uint8_t idx)
{
	reply_to_pc(VTT_RSP_ACK, idx, 0u, 0u, 0u, 0u, 0u, 0u);
}

/* ------------------------------------------------------------------------- */
/* §10.2 — three DATA frames (public: main enqueues after ACK was drained). */
/* ------------------------------------------------------------------------- */

void comm_enqueue_measurement_data(
	uint8_t idx,
	uint16_t uh_sum,
	uint16_t ih_sum,
	uint16_t ua_sum,
	uint16_t ug2_sum,
	uint16_t ug1_sum,
	uint16_t ia_sum,
	uint16_t ig2_sum,
	uint8_t alarm)
{
	/* Frame 1: Uh, Ih, reserved */
	reply_to_pc(
	    VTT_RSP_DATA,
	    idx,
	    le16_lo(uh_sum),
	    le16_hi(uh_sum),
	    le16_lo(ih_sum),
	    le16_hi(ih_sum),
	    0u,
	    0u);

	/* Frame 2: Ua, Ug2, Ug1 */
	reply_to_pc(
	    VTT_RSP_DATA,
	    idx,
	    le16_lo(ua_sum),
	    le16_hi(ua_sum),
	    le16_lo(ug2_sum),
	    le16_hi(ug2_sum),
	    le16_lo(ug1_sum),
	    le16_hi(ug1_sum));

	/* Frame 3: Ia, Ig2, alarm, reserved */
	reply_to_pc(
	    VTT_RSP_DATA,
	    idx,
	    le16_lo(ia_sum),
	    le16_hi(ia_sum),
	    le16_lo(ig2_sum),
	    le16_hi(ig2_sum),
	    alarm,
	    0u);
}

uint8_t comm_measurement_data_pending(void)
{
	return measurement_data_pending;
}

void comm_get_last_remote_set(
	uint8_t *idx,
	uint8_t *p1_heater,
	uint16_t *ua_12,
	uint16_t *ug2_12,
	uint8_t *p5_ug1_mag,
	uint8_t *p6_time_idx)
{
	*idx = last_set_idx;
	*p1_heater = last_set_p1_heater;
	*ua_12 = last_set_ua_12;
	*ug2_12 = last_set_ug2_12;
	*p5_ug1_mag = last_set_p5_ug1;
	*p6_time_idx = last_set_p6_time;
}

void comm_clear_measurement_data_pending(void)
{
	measurement_data_pending = 0;
	vt_meas_busy = 0;
}

/* ------------------------------------------------------------------------- */
/* SET validation (v0.4.1 §6): P2–P4 = 12-bit Ua/Ug2 */
/* ------------------------------------------------------------------------- */



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

static int set_params_ok(const frame_t *rq)
{
	uint16_t ua;
	uint16_t ug2;

	if (!index_ok(rq->idx))
	{
		reply_error_invalid(rq->idx);
		return 0;
	}

	ua = (uint16_t)rq->p2 | ((uint16_t)((rq->p3 >> 4) & 0x0Fu) << 8);
	ug2 = (uint16_t)rq->p4 | ((uint16_t)(rq->p3 & 0x0Fu) << 8);
	if (ua > VTT_UA_UG2_MAX)
	{
		reply_error_range(rq->idx, 2u, ua);
		return 0;
	}
	if (ug2 > VTT_UA_UG2_MAX)
	{
		reply_error_range(rq->idx, 2u, ug2);
		return 0;
	}

	if (rq->p5 > 240u)
	{
		reply_error_range(rq->idx, 5u, rq->p5);
		return 0;
	}
	if (rq->p6 > 63u)
	{
		reply_error_range(rq->idx, 6u, rq->p6);
		return 0;
	}
	return 1;
}

/* ------------------------------------------------------------------------- */

/*
 * One validated 9-byte request from the PC (already CRC-checked in comm_receive_byte,
 * staged into staged_host_request; run from comm_handle_requests, not from ISR).
 * Low 6 bits of CTRL must be 0 (v0.4.1 §3). Dispatches SET / STATUS / RESET and
 * enqueues replies via reply_to_pc / reply_error. DATA frames: main calls
 * comm_enqueue_measurement_data after ACK (see protocol flow / heating delay).
 */
static void handle_request(const frame_t *rq)
{
	uint8_t cmd = (uint8_t)(rq->ctrl & 0xC0u);

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
			return;
		{
			uint16_t ua;
			uint16_t ug2;

			ua = (uint16_t)rq->p2 | ((uint16_t)((rq->p3 >> 4) & 0x0Fu) << 8);
			ug2 = (uint16_t)rq->p4 | ((uint16_t)(rq->p3 & 0x0Fu) << 8);
			last_set_idx = rq->idx;
			last_set_p1_heater = rq->p1;
			last_set_ua_12 = ua;
			last_set_ug2_12 = ug2;
			last_set_p5_ug1 = rq->p5;
			last_set_p6_time = rq->p6;
			measurement_data_pending = 1u;
			vt_meas_busy = 1u;
		}
		reply_ack(rq->idx);
		break;

	case VTT_REQ_STATUS:
		reply_to_pc(
		    vt_meas_busy ? VTT_RSP_BUSY : VTT_RSP_ACK,
		    rq->idx,
		    0u,
		    0u,
		    0u,
		    0u,
		    0u,
		    0u);
		break;

	case VTT_REQ_RESET:
	{
		uint8_t p1 = rq->p1;

		if (p1 != VTT_RESET_UA_UG2_UG1 && p1 != VTT_RESET_FULL && p1 != 0u)
			p1 = VTT_RESET_FULL;
		(void)p1;
		measurement_data_pending = 0;
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

/*
 * Feed one raw RX byte (e.g. from USART RX ISR). Safe to call from interrupt context;
 * do not call comm_handle_requests from the ISR.
 *
 * Collects VTT_FRAME_BYTES into incoming_raw_bytes; on overflow, resyncs to a new frame
 * starting with the current byte. When a full frame is present, verifies CRC over bytes
 * 0–7 vs byte 8 (v0.4.1 §2.2). Valid frame is copied to staged_host_request if the slot
 * is free; otherwise it is dropped until the main loop drains the previous request.
 */
void comm_receive_byte(uint8_t b)
{
	if (incoming_byte_count < VTT_FRAME_BYTES)
		incoming_raw_bytes[incoming_byte_count++] = b;
	else
	{
		incoming_raw_bytes[0] = b;
		incoming_byte_count = 1u;
	}

	if (incoming_byte_count != VTT_FRAME_BYTES)
		return;

	if (comm_checksum(incoming_raw_bytes) != incoming_raw_bytes[8])
	{
		crc_error_from_host_flag = 1u;
		incoming_byte_count = 0;
		return;
	}

	if (!host_request_ready)
	{
		/* Handled in comm_handle_requests → handle_request (main loop, not ISR). */
		memcpy(&staged_host_request, incoming_raw_bytes, sizeof(frame_t));
		host_request_ready = 1u;
	}
	incoming_byte_count = 0;
}

/*
 * Main-loop / foreground work only: must not run inside RX ISR.
 *
 * 1) If the last assembled frame failed CRC, enqueue a host-visible ERROR (CRC) reply.
 * 2) If a request was staged by comm_receive_byte, run handle_request (may enqueue many frames).
 * 3) Drain the outgoing queue on the UART byte-by-byte via comm_transmit_byte (blocking TX).
 */
void comm_handle_requests(void)
{
	frame_t out;
	uint8_t i;

	// 1) If the last assembled frame failed CRC, enqueue a host-visible ERROR (CRC) reply.
	if (crc_error_from_host_flag)
	{
		crc_error_from_host_flag = 0u;
		reply_error_crc();
	}

	// 2) If a request was staged by comm_receive_byte, run handle_request (may enqueue many frames).
	if (host_request_ready)
	{
		out = staged_host_request;
		host_request_ready = 0u;
		handle_request(&out);
	}

	// 3) Drain the outgoing queue on the UART byte-by-byte via comm_transmit_byte (blocking TX).
	while (outgoing_queued_frame_count > 0u)
	{
		const uint8_t *bytes;

		out = outgoing_frame_queue[outgoing_queue_read_index];
		outgoing_queue_read_index =
		    (uint8_t)((outgoing_queue_read_index + 1u) % OUTGOING_FRAME_QUEUE_DEPTH);
		outgoing_queued_frame_count--;

		bytes = (const uint8_t *)&out;
		for (i = 0; i < VTT_FRAME_BYTES; i++)
			comm_transmit_byte(bytes[i]);
	}
}
