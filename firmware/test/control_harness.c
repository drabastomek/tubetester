/*
 * Harness for control unit tests: defines all externs and stubs (load_lamprom, AZ).
 * Use with test config (katalog, FLAMP, ELAMP, EEPROM stubs).
 */
#include "config/config.h"
#include <string.h>

/* Control externs (names match FIRMWARE_VARIABLE_REFERENCE.md) */
uint8_t lcd_line_buffer[64], edit_field_index, encoder_new_turn_flag, debounce_encoder_ticks_stable_cnt, debounce_set_button_hold_ticks, save_eeprom_flag, read_eeprom_flag;
uint16_t tube_type_index;
uint8_t ia_range_hi, ia_range_lcd, ia_range_default, tx_send_flag, legacy_rx_buffer[10];
uint16_t sequencer_phase_ticks, heating_time_ticks, adc_vref_scaled;
uint16_t setpoint_uh, setpoint_ih, setpoint_ug1, setpoint_ua, setpoint_ia, setpoint_ug2, setpoint_ig2;
uint16_t adc_mean_uh, adc_mean_ih, adc_mean_ug1, adc_mean_ua, adc_mean_ia, adc_mean_ug2, adc_mean_ig2;
uint16_t ug1_dac_zero, ug1_dac_ref, meas_uh, meas_ih, meas_ua, meas_ia, meas_ug2, meas_ig2, meas_ug1;
uint16_t lcd_uh, lcd_ih, lcd_ug1, lcd_ua, lcd_ia, lcd_ug2, lcd_ig2, lcd_s, lcd_r, lcd_k;
uint16_t slope_s, resistance_r, amplification_k, tube_type_index_prev;
uint32_t ug1_calc_accum, ug1_calc_temp;
uint8_t anode_system_select;
katalog catalog_remote, catalog_current;
uint16_t eeprom_last_tube_type;
katalog catalog_eeprom[ELAMP];

/* char_lookup_alphabet: index 0..25 A-Z, 26=_, 27..35 '0'..'9' (tube_name bytes are indices) */
const uint8_t char_lookup_alphabet[37] = {
   'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
   '_', '0','1','2','3','4','5','6','7','8','9'
};

/* Minimal ROM catalog: index 0=PWR SUP, 1=REMOTE, 2=6N1P (tube_name as ASCII; tube_name[8] used for heating_time_ticks) */
static const katalog catalog_rom[FLAMP] = {
   { {'P','W','R',' ','S','U','P',' ','0'}, 0, 0, 240, 0, 0, 0, 0, 0, 0, 0 },
   { {'R','E','M','O','T','E',' ',' ','1'}, 0, 0, 240, 0, 0, 0, 0, 0, 0, 0 },
   { {'6','N','1','P',' ',' ','G','1','1'}, 63, 0, 40, 250, 75, 0, 0, 45, 8, 35 },
};

void load_lamprom(uint16_t idx, katalog *dest)
{
   if (idx < FLAMP)
      memcpy(dest, &catalog_rom[idx], sizeof(katalog));
}
