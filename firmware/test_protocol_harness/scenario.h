#ifndef SCENARIO_H
#define SCENARIO_H

#include <stdint.h>

/* Arm via STATUS with A1_A2='H', ih byte = fault id (harness-only). */
#define HARNS_FAULT_ALARM_ON_SET    1u
#define HARNS_FAULT_SILENT_ONCE     2u
#define HARNS_FAULT_HWERR_ON_STATUS 3u

void scenario_init(void);
void scenario_poll(void);

void scenario_arm_fault(uint8_t fault_id);
uint8_t scenario_peek_fault(void);

void scenario_on_set_accepted(uint8_t a1_a2);
void scenario_send_status_data(uint8_t a1_a2);

/* Returns 1 if the command was handled (e.g. silent discard) with no TX. */
uint8_t scenario_apply_inbound_fault(void);

/* Returns 1 if fault replaced the normal SET reply (alarm sent). */
uint8_t scenario_apply_set_fault(void);

/* Returns 1 if fault replaced the normal STATUS reply. */
uint8_t scenario_apply_status_fault(void);

#endif
