/*
 * VTTester SET command: decode 8-byte SET frame into firmware parameters.
 * Validates ranges; returns VTTESTER_ERR_OK or VTTESTER_ERR_OUT_OF_RANGE.
 */

#ifndef VTTESTER_SETUP_H
#define VTTESTER_SETUP_H

#include "vttester_remote.h"

/* Decoded SET parameters to apply to lamptem + tuh.
 * uadef, ug2def in volts (0..300). ug1def 0..240 (-24V..0). uhdef 0..200 (0..20V). ihdef 0..250 (0..2.5A).
 * tuh_ticks = warmup in 250ms ticks (set global tuh).
 */
typedef struct {
    unsigned char uhdef;
    unsigned char ihdef;
    unsigned char ug1def;
    unsigned int  uadef;
    unsigned int  ug2def;
    unsigned int  tuh_ticks;
    /* On OUT_OF_RANGE: which param (1..5) and attempted value (for ACK R2,R3,R4) */
    unsigned char error_param;
    unsigned int  error_value;
} vttester_set_params_t;

/* Decode SET frame (bytes 0..7) into out. Validate; on invalid param set out->error_param (1..5), out->error_value and return VTTESTER_ERR_OUT_OF_RANGE. */
unsigned char vttester_setup_decode(const unsigned char *frame, vttester_set_params_t *out);

#endif /* VTTESTER_SETUP_H */
