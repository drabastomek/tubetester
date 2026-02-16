/*
 * Control logic: menu, katalog, parameter editing, calculations.
 * Fills lcd_line_buffer[] from ADC/state and handles encoder/EEPROM/setpoints.
 * Call control_update(ascii) each main_loop_sync_flag tick after display_refresh().
 */

#ifndef VTTESTER_CONTROL_H
#define VTTESTER_CONTROL_H

#include <stdint.h>

void control_update(uint8_t *ascii);

#endif /* VTTESTER_CONTROL_H */
