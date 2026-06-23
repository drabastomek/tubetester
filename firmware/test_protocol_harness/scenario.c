#include "communication/communication.h"
#include "harness_config.h"
#include "harness_tick.h"
#include "scenario.h"
#include "synthetic_measure.h"

static uint8_t s_pending_auto_data;
static uint8_t s_auto_a1_a2;
static uint16_t s_auto_deadline_ms;
static uint8_t s_armed_fault;

void scenario_init(void)
{
	s_pending_auto_data = 0u;
	s_auto_a1_a2 = (uint8_t)'0';
	s_auto_deadline_ms = 0u;
	s_armed_fault = 0u;
}

void scenario_arm_fault(uint8_t fault_id)
{
	s_armed_fault = fault_id;
}

uint8_t scenario_peek_fault(void)
{
	return s_armed_fault;
}

uint8_t scenario_apply_inbound_fault(void)
{
	if (s_armed_fault != HARNS_FAULT_SILENT_ONCE)
		return 0u;

	s_armed_fault = 0u;
	return 1u;
}

uint8_t scenario_apply_set_fault(void)
{
	if (s_armed_fault != HARNS_FAULT_ALARM_ON_SET)
		return 0u;

	s_armed_fault = 0u;
	send_alarm(OVERIH);
	return 1u;
}

uint8_t scenario_apply_status_fault(void)
{
	if (s_armed_fault != HARNS_FAULT_HWERR_ON_STATUS)
		return 0u;

	s_armed_fault = 0u;
	send_error(VTT_ERR_HARDWARE);
	return 1u;
}

void scenario_send_status_data(uint8_t a1_a2)
{
	uint16_t samples[8];
	const synthetic_measure_t *m = synthetic_measure_get();

	synthetic_measure_fill_data(samples, 8u);
	send_data(a1_a2, samples, 8u, m->err);
}

void scenario_on_set_accepted(uint8_t a1_a2)
{
#if HARNESS_FLOW == HARNESS_FLOW_AUTO_MEASURE
	s_auto_a1_a2 = a1_a2;
	s_auto_deadline_ms = harness_millis() + HARNESS_MEAS_DELAY_MS;
	s_pending_auto_data = 1u;
#else
	(void)a1_a2;
#endif
}

void scenario_poll(void)
{
#if HARNESS_FLOW == HARNESS_FLOW_AUTO_MEASURE
	if (!s_pending_auto_data)
		return;
	if (harness_millis() < s_auto_deadline_ms)
		return;

	s_pending_auto_data = 0u;
	scenario_send_status_data(s_auto_a1_a2);
#endif
}
