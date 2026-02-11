/*
 * VTTester SET decode: P1 (heater) -> uhdef/ihdef, P2/P3 -> uadef/ug2def,
 * P4 -> ug1def, P5 -> tuh. Simple encoding: P2/P3 = index*10V, P4 = index*0.5V (negated).
 */

#include "vttester_setup.h"

/* HEAT_IDX 0..15 -> heater voltage (0.1V). 0=off, 1=1.4V, 2=2.0, 3=2.5, 4=4.0, 5=5.0, 6=6.3, 7=12.6. */
static const unsigned char heat_voltage[8] = { 0, 14, 20, 25, 40, 50, 63, 126 };

unsigned char vttester_setup_decode(const unsigned char *frame, vttester_set_params_t *out)
{
    unsigned char p1, p2, p3, p4, p5;
    unsigned char heat_idx;
    unsigned int ua_v, ug2_v;
    unsigned int ug1_mag;  /* magnitude in 0.1V */

    out->error_param = 0;
    out->error_value = 0;

    p1 = frame[2];
    p2 = frame[3];
    p3 = frame[4];
    p4 = frame[5];
    p5 = frame[6];

    heat_idx = p1 & 0x0F;
    if (heat_idx > 7) {
        out->error_param = 1;
        out->error_value = heat_idx;
        return VTTESTER_ERR_OUT_OF_RANGE;
    }
    out->uhdef = heat_voltage[heat_idx];
    out->ihdef = 0;

    ua_v = (unsigned int)p2 * 10u;
    if (ua_v > 300u) {
        out->error_param = 2;
        out->error_value = ua_v;
        return VTTESTER_ERR_OUT_OF_RANGE;
    }
    out->uadef = ua_v;

    ug2_v = (unsigned int)p3 * 10u;
    if (ug2_v > 300u) {
        out->error_param = 3;
        out->error_value = ug2_v;
        return VTTESTER_ERR_OUT_OF_RANGE;
    }
    out->ug2def = ug2_v;

    /* Ug1 = -P4 * 0.5 V. ug1def: 240 = -24V, 0 = 0V. Magnitude 0.5*P4 V = 5*P4 in 0.1V. */
    ug1_mag = (unsigned int)p4 * 5u;
    if (ug1_mag > 240u) {
        out->error_param = 4;
        out->error_value = (unsigned int)p4;
        return VTTESTER_ERR_OUT_OF_RANGE;
    }
    out->ug1def = 240 - (unsigned char)ug1_mag;

    if (p5 > 63) {
        out->error_param = 5;
        out->error_value = p5;
        return VTTESTER_ERR_OUT_OF_RANGE;
    }
    out->tuh_ticks = (unsigned int)p5 * 2u;

    return VTTESTER_ERR_OK;
}
