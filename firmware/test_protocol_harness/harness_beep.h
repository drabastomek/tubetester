#ifndef HARNESS_BEEP_H
#define HARNESS_BEEP_H

#include <stdint.h>

/* PORTD bit 7 — SPK (see config.c pin comment). */
void harness_beep_play(uint8_t dur_units);

#endif
