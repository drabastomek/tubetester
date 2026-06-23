#include "communication/communication.h"
#include "config/config.h"
#include "harness_command.h"
#include "harness_config.h"
#include "scenario.h"

#include <util/delay.h>

static harness_params_t s_params;

static void harness_tx_delay(void)
{
#if HARNESS_TX_DELAY_MS > 0u
	_delay_ms(HARNESS_TX_DELAY_MS);
#endif
}

static uint8_t validate_parameters(const uint8_t *rq)
{
	uint16_t temp;

	if (rq[2] < UG_MIN || rq[2] > UG_MAX) {
		send_input_range_error(PARAM_ID_UG, rq[2]);
		return 0u;
	}
	if (rq[3] < UH_MIN || rq[3] > UH_MAX) {
		send_input_range_error(PARAM_ID_UH, rq[3]);
		return 0u;
	}
	if (rq[4] < IH_MIN || rq[4] > IH_MAX) {
		send_input_range_error(PARAM_ID_IH, rq[4]);
		return 0u;
	}

	temp = (uint16_t)rq[5] | ((uint16_t)rq[6] << 8);
	if (temp < UA_MIN || temp > UA_MAX) {
		send_input_range_error(PARAM_ID_UA, rq[6]);
		return 0u;
	}

	temp = (uint16_t)rq[7] | ((uint16_t)rq[8] << 8);
	if (temp < UE_MIN || temp > UE_MAX) {
		send_input_range_error(PARAM_ID_UE, rq[8]);
		return 0u;
	}

	return 1u;
}

const harness_params_t *harness_params_get(void)
{
	return &s_params;
}

void harness_command_handle(const uint8_t *rq)
{
	uint8_t cmd;

	cmd = rq[0];

	switch (cmd) {
	case CMD_SET:
		if (!validate_parameters(rq))
			return;

		s_params.a1_a2 = rq[1];
		s_params.ugset = rq[2];
		s_params.uhset = rq[3];
		s_params.ihset = rq[4];
		s_params.uaset = (uint16_t)rq[5] | ((uint16_t)rq[6] << 8);
		s_params.ueset = (uint16_t)rq[7] | ((uint16_t)rq[8] << 8);

		harness_tx_delay();
		send_ack();
		scenario_on_set_accepted(s_params.a1_a2);
		break;

	case CMD_RESET:
		if (rq[1] == RESET_HEATER) {
			s_params.uhset = 0u;
			s_params.ihset = 0u;
		} else {
			s_params.uaset = 0u;
			s_params.ueset = 0u;
			s_params.ugset = 240u;
		}

		harness_tx_delay();
		send_ack();
		break;

	case CMD_STATUS:
		harness_tx_delay();
		scenario_send_status_data(s_params.a1_a2);
		break;

	default:
		harness_tx_delay();
		send_error(VTT_ERR_INVALID_CMD);
		break;
	}
}
