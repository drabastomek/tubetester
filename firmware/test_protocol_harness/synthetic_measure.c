#include "synthetic_measure.h"

static synthetic_measure_t s_meas;

void synthetic_measure_init(void)
{
	synthetic_measure_load_nominal();
}

void synthetic_measure_load_nominal(void)
{
	s_meas.ug_sum = (uint16_t)(20.1f * 64.0f);
	s_meas.uh_sum = (uint16_t)(126.1f * 64.0f);
	s_meas.ih_sum = (uint16_t)(30.5f * 64.0f);
	s_meas.ua_sum = (uint16_t)(256.4f * 64.0f);
	s_meas.ia_sum = (uint16_t)(15.1f * 64.0f);
	s_meas.ue_sum = 0u;
	s_meas.ie_sum = 0u;
	s_meas.tm_sum = (uint16_t)(12.0f * 64.0f);
	s_meas.err = 0u;
}

const synthetic_measure_t *synthetic_measure_get(void)
{
	return &s_meas;
}

void synthetic_measure_set(const synthetic_measure_t *m)
{
	if (m)
		s_meas = *m;
}

void synthetic_measure_fill_data(uint16_t *out, uint8_t out_len)
{
	if (!out || out_len < 8u)
		return;

	/* v0.5 §5 wire order: Ug, Uh, Ih, Ua, Ia, Ue, Ie, Tm. */
	out[0] = s_meas.ug_sum;
	out[1] = s_meas.uh_sum;
	out[2] = s_meas.ih_sum;
	out[3] = s_meas.ua_sum;
	out[4] = s_meas.ia_sum;
	out[5] = s_meas.ue_sum;
	out[6] = s_meas.ie_sum;
	out[7] = s_meas.tm_sum;
}
