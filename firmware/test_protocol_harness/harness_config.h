#ifndef HARNESS_CONFIG_H
#define HARNESS_CONFIG_H

/*
 * Protocol test harness — build-time options and defaults.
 * UART baud comes from config/config.h (RATE → 9600 @ 16 MHz).
 */

#include "config/config.h"

/* SET → ACK only; DATA on explicit STATUS (matches original comms_test stub). */
#define HARNESS_FLOW_SPLIT 0u

/* SET → ACK, then auto DATA after HARNESS_MEAS_DELAY_MS. */
#define HARNESS_FLOW_AUTO_MEASURE 1u

#ifndef HARNESS_FLOW
#define HARNESS_FLOW HARNESS_FLOW_SPLIT
#endif

/* Used when HARNESS_FLOW == HARNESS_FLOW_AUTO_MEASURE. */
#ifndef HARNESS_MEAS_DELAY_MS
#define HARNESS_MEAS_DELAY_MS 50u
#endif

/* Millisecond tick for non-blocking delays (Timer0). */
#ifndef HARNESS_TICK_MS
#define HARNESS_TICK_MS 1u
#endif

/* Optional TX delay before any reply (fault-injection hook; 0 = off). */
#ifndef HARNESS_TX_DELAY_MS
#define HARNESS_TX_DELAY_MS 0u
#endif

#endif
