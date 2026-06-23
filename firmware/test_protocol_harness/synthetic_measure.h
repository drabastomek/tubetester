#ifndef SYNTHETIC_MEASURE_H
#define SYNTHETIC_MEASURE_H

#include <stdint.h>

typedef struct {
	uint16_t ug_sum;
	uint16_t uh_sum;
	uint16_t ih_sum;
	uint16_t ua_sum;
	uint16_t ia_sum;
	uint16_t ue_sum;
	uint16_t ie_sum;
	uint16_t tm_sum;
	uint8_t err;
} synthetic_measure_t;

void synthetic_measure_init(void);
void synthetic_measure_load_nominal(void);

const synthetic_measure_t *synthetic_measure_get(void);
void synthetic_measure_set(const synthetic_measure_t *m);

/* Fill v0.5 §5 channel order for send_data(): Ug, Uh, Ih, Ua, Ia, Ue, Ie, Tm. */
void synthetic_measure_fill_data(uint16_t *out, uint8_t out_len);

#endif
