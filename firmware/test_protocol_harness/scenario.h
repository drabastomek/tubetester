#ifndef SCENARIO_H
#define SCENARIO_H

#include <stdint.h>

void scenario_init(void);
void scenario_poll(void);

void scenario_on_set_accepted(uint8_t a1_a2);
void scenario_send_status_data(uint8_t a1_a2);

#endif
