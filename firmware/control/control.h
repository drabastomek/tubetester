/*
 * Control logic: menu, katalog, parameter editing, calculations.
 * Fills buf[] from ADC/state and handles encoder/EEPROM/setpoints.
 * Call control_update(ascii) each sync tick after display_refresh().
 */

#ifndef VTTESTER_CONTROL_H
#define VTTESTER_CONTROL_H

void control_update(unsigned char *ascii);

#endif /* VTTESTER_CONTROL_H */
