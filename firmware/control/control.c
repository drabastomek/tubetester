/*
 * Control logic: menu, katalog, parameter editing, calculations.
 * Uses extern state from main; fills lcd_line_buffer[] and updates setpoints/EEPROM.
 */

#include "config/config.h"
#include "control/control.h"
#include "utils/utils.h"
#ifndef VTTESTER_HOST_TEST
#if defined(ICCAVR)
#include <iom32v.h>
#else
#include <avr/io.h>
#endif
#endif

extern uint8_t lcd_line_buffer[64], edit_field_index, encoder_new_turn_flag, debounce_encoder_ticks_stable_cnt, debounce_set_button_hold_ticks, save_eeprom_flag, read_eeprom_flag;
extern uint8_t ia_range_hi, ia_range_lcd, ia_range_default, tx_send_flag, legacy_rx_buffer[10];
extern uint16_t tube_type_index, sequencer_phase_ticks, heating_time_ticks, adc_vref_scaled;
extern uint16_t setpoint_uh, setpoint_ih, setpoint_ug1, setpoint_ua, setpoint_ug2;
extern uint16_t adc_mean_uh, adc_mean_ih, adc_mean_ug1, adc_mean_ua, adc_mean_ia, adc_mean_ug2, adc_mean_ig2;
extern uint16_t ug1_dac_zero, ug1_dac_ref, meas_uh, meas_ih, meas_ua, meas_ia, meas_ug2, meas_ig2, meas_ug1;
extern uint16_t lcd_uh, lcd_ih, lcd_ug1, lcd_ua, lcd_ia, lcd_ug2, lcd_ig2, lcd_s, lcd_r, lcd_k;
extern uint16_t slope_s, resistance_r, amplification_k, tube_type_index_prev;
extern uint32_t ug1_calc_accum, ug1_calc_temp;
extern uint8_t anode_system_select;
extern katalog catalog_remote, catalog_current;
extern uint16_t eeprom_last_tube_type;
extern const uint8_t char_lookup_alphabet[37];
extern void load_lamprom(uint16_t idx, katalog *dest);
extern katalog catalog_eeprom[ELAMP];
extern void char2rs(uint8_t bajt);
extern void cstr2rs(const char *q);

void control_update(uint8_t *ascii)
{
   uint8_t i;

   /* Wyswietlanie Numeru / Pobieranie nowej Nazwy / Edycja Nazwy */
   if (edit_field_index == 0) {
      int2asc(tube_type_index, ascii);
      lcd_line_buffer[0] = ascii[2];
      lcd_line_buffer[1] = ascii[1];
      lcd_line_buffer[2] = ascii[0];

      if (tube_type_index < FLAMP) {
         if (tube_type_index == 1) {
            catalog_current = catalog_remote;
         } else {
            load_lamprom(tube_type_index, &catalog_current);
            if ((tube_type_index != tube_type_index_prev) && (sequencer_phase_ticks != (FUH+2))) {
               load_lamprom(1, &catalog_remote);
               anode_system_select = 28;
               setpoint_ug1 = ug1_dac_zero = ug1_dac_ref = liczug1(240);
               setpoint_uh = setpoint_ih = setpoint_ua = setpoint_ug2 = 0;
            }
         }
         heating_time_ticks = (catalog_current.tube_name[8] - '0') * 240;
         for (i = 0; i < 9; i++) lcd_line_buffer[i+4] = (uint8_t)catalog_current.tube_name[i];
      } else {
         EEPROM_READ(&catalog_eeprom[tube_type_index-FLAMP], catalog_current);
         heating_time_ticks = (catalog_current.tube_name[8] - 27) * 240;
         for (i = 0; i < 9; i++) lcd_line_buffer[i+4] = char_lookup_alphabet[(uint8_t)catalog_current.tube_name[i]];
      }
      tube_type_index_prev = tube_type_index;
      encoder_new_turn_flag = 1;
   }

   if ((tube_type_index == 0) || (tube_type_index == 1)) {
      anode_system_select = catalog_current.tube_name[7];
      lcd_line_buffer[11] = char_lookup_alphabet[anode_system_select];
   }

   if (tube_type_index >= FLAMP) {
      if ((edit_field_index > 0) && (edit_field_index < 10)) {
         if (read_eeprom_flag == 1) EEPROM_READ(&catalog_eeprom[tube_type_index-FLAMP].tube_name, catalog_current.tube_name);
         for (i = 0; i < 9; i++) lcd_line_buffer[i+4] = char_lookup_alphabet[(uint8_t)catalog_current.tube_name[i]];
         read_eeprom_flag = 0;
         if (debounce_set_button_hold_ticks == DMAX) save_eeprom_flag = 1;
         if ((debounce_encoder_ticks_stable_cnt == DMIN) && (save_eeprom_flag == 1)) {
            EEPROM_WRITE(&catalog_eeprom[tube_type_index-FLAMP].tube_name[edit_field_index-1], catalog_current.tube_name[edit_field_index-1]);
            save_eeprom_flag = 0;
         }
      } else {
         read_eeprom_flag = 1;
      }
   }

   /* Ug1 */
   ug1_dac_ref = liczug1(catalog_current.voltage_g1_set);
   ug1_calc_temp = adc_mean_ug1;
   ug1_calc_temp *= 725;
   ug1_calc_accum = 40960000;
   if (ug1_calc_accum > ug1_calc_temp) ug1_calc_accum -= ug1_calc_temp; else ug1_calc_accum = 0;
   ug1_calc_accum >>= 16;
   ug1_calc_accum *= adc_vref_scaled;
   ug1_calc_accum /= 1000;
   meas_ug1 = (uint16_t)ug1_calc_accum;
   if (sequencer_phase_ticks == (FUH+2)) ug1_calc_accum = lcd_ug1;
   if ((edit_field_index == 0) || (edit_field_index == 10)) {
      if (debounce_set_button_hold_ticks == DMAX) { ug1_calc_accum = catalog_current.voltage_g1_set; save_eeprom_flag = 1; }
      if ((debounce_encoder_ticks_stable_cnt == DMIN) && (edit_field_index == 10) && (save_eeprom_flag == 1)) {
         save_eeprom_flag = 0;
         if (tube_type_index == 0) setpoint_ug1 = ug1_dac_ref;
         if (tube_type_index >= FLAMP) EEPROM_WRITE(&catalog_eeprom[tube_type_index-FLAMP].voltage_g1_set, catalog_current.voltage_g1_set);
      }
   }
   int2asc((uint16_t)ug1_calc_accum, ascii);
   lcd_line_buffer[24] = (ascii[2] != '0') ? ascii[2] : ' ';
   lcd_line_buffer[25] = ascii[1];
   lcd_line_buffer[26] = '.';
   lcd_line_buffer[27] = ascii[0];

   /* Uh */
   ug1_calc_accum = adc_mean_uh;
   ug1_calc_accum *= adc_vref_scaled;
   ug1_calc_accum >>= 14;
   ug1_calc_temp = adc_mean_ih;
   ug1_calc_temp *= adc_vref_scaled;
   ug1_calc_temp >>= 16;
   if (ug1_calc_accum > ug1_calc_temp) ug1_calc_accum -= ug1_calc_temp; else ug1_calc_accum = 0;
   ug1_calc_accum /= 10;
   meas_uh = (uint16_t)ug1_calc_accum;
   if (sequencer_phase_ticks == (FUH+2)) ug1_calc_accum = lcd_uh;
   if ((edit_field_index == 0) || (edit_field_index == 11)) {
      if (debounce_set_button_hold_ticks == DMAX) { ug1_calc_accum = catalog_current.voltage_heater_set; save_eeprom_flag = 1; }
      if ((debounce_encoder_ticks_stable_cnt == DMIN) && (edit_field_index == 11) && (save_eeprom_flag == 1)) {
         save_eeprom_flag = 0;
         if (tube_type_index == 0) { setpoint_uh = catalog_current.voltage_heater_set; setpoint_ih = catalog_current.current_heater_set = 0; }
         if (tube_type_index >= FLAMP) EEPROM_WRITE(&catalog_eeprom[tube_type_index-FLAMP].voltage_heater_set, catalog_current.voltage_heater_set);
      }
   }
   int2asc((uint16_t)ug1_calc_accum, ascii);
   lcd_line_buffer[14] = (ascii[2] != '0') ? ascii[2] : ' ';
   lcd_line_buffer[15] = ascii[1];
   lcd_line_buffer[16] = '.';
   lcd_line_buffer[17] = ascii[0];

   /* Ih */
   ug1_calc_accum = adc_mean_ih;
   ug1_calc_accum *= adc_vref_scaled;
   ug1_calc_accum >>= 15;
   meas_ih = (uint16_t)ug1_calc_accum;
   if (sequencer_phase_ticks == (FUH+2)) ug1_calc_accum = lcd_ih;
   if ((edit_field_index == 0) || (edit_field_index == 12)) {
      if (debounce_set_button_hold_ticks == DMAX) { ug1_calc_accum = catalog_current.current_heater_set; save_eeprom_flag = 1; }
      if ((debounce_encoder_ticks_stable_cnt == DMIN) && (edit_field_index == 12) && (save_eeprom_flag == 1)) {
         save_eeprom_flag = 0;
         if (tube_type_index == 0) { setpoint_ih = catalog_current.current_heater_set; setpoint_uh = catalog_current.voltage_heater_set = 0; }
         if (tube_type_index >= FLAMP) EEPROM_WRITE(&catalog_eeprom[tube_type_index-FLAMP].current_heater_set, catalog_current.current_heater_set);
      }
   }
   int2asc((uint16_t)ug1_calc_accum, ascii);
   if (ascii[2] != '0') {
      lcd_line_buffer[19] = ascii[2]; lcd_line_buffer[20] = ascii[1]; lcd_line_buffer[21] = ascii[0];
   } else {
      lcd_line_buffer[19] = ' ';
      if (ascii[1] != '0') { lcd_line_buffer[20] = ascii[1]; lcd_line_buffer[21] = ascii[0]; }
      else { lcd_line_buffer[20] = ' '; lcd_line_buffer[21] = (ascii[0] != '0') ? ascii[0] : ' '; }
   }
   lcd_line_buffer[22] = '0';

   /* Ua */
   ug1_calc_accum = adc_mean_ua;
   ug1_calc_accum *= adc_vref_scaled;
   ug1_calc_accum /= 107436;
   meas_ua = (uint16_t)ug1_calc_accum;
   if (sequencer_phase_ticks == (FUH+2)) ug1_calc_accum = lcd_ua;
   if ((edit_field_index == 0) || (edit_field_index == 13)) {
      if (debounce_set_button_hold_ticks == DMAX) { ug1_calc_accum = catalog_current.voltage_anode_set; save_eeprom_flag = 1; }
      if ((debounce_encoder_ticks_stable_cnt == DMIN) && (edit_field_index == 13) && (save_eeprom_flag == 1)) {
         save_eeprom_flag = 0;
         if (tube_type_index == 0) setpoint_ua = catalog_current.voltage_anode_set;
         if (tube_type_index >= FLAMP) EEPROM_WRITE(&catalog_eeprom[tube_type_index-FLAMP].voltage_anode_set, catalog_current.voltage_anode_set);
      }
   }
   int2asc((uint16_t)ug1_calc_accum, ascii);
   if (ascii[2] != '0') { lcd_line_buffer[29] = ascii[2]; lcd_line_buffer[30] = ascii[1]; }
   else { lcd_line_buffer[29] = ' '; lcd_line_buffer[30] = (ascii[1] != '0') ? ascii[1] : ' '; }
   lcd_line_buffer[31] = ascii[0];

   /* Ia */
   ug1_calc_accum = adc_mean_ia;
   ug1_calc_accum *= adc_vref_scaled;
   ug1_calc_accum >>= 14;
   ug1_calc_temp = adc_mean_ua;
   ug1_calc_temp *= adc_vref_scaled;
   if (ia_range_hi == 0) ug1_calc_temp *= 10;
   ug1_calc_temp /= 4369064;
   if (ug1_calc_accum > ug1_calc_temp) ug1_calc_accum -= ug1_calc_temp; else ug1_calc_accum = 0;
   meas_ia = (uint16_t)ug1_calc_accum;
   if (sequencer_phase_ticks == (FUH+2)) ug1_calc_accum = lcd_ia;
   ia_range_default = 0;
   if ((edit_field_index == 0) || (edit_field_index == 14)) {
      if (debounce_set_button_hold_ticks == DMAX) { ug1_calc_accum = catalog_current.current_anode_ref; ia_range_default = 1; save_eeprom_flag = 1; }
      if ((debounce_encoder_ticks_stable_cnt == DMIN) && (edit_field_index == 14) && (save_eeprom_flag == 1)) {
         save_eeprom_flag = 0;
         if (tube_type_index >= FLAMP) EEPROM_WRITE(&catalog_eeprom[tube_type_index-FLAMP].current_anode_ref, catalog_current.current_anode_ref);
      }
   }
   int2asc((uint16_t)ug1_calc_accum, ascii);
   if ((ia_range_default == 0) && (((sequencer_phase_ticks != (FUH+2)) && (ia_range_hi == 0)) || ((sequencer_phase_ticks == (FUH+2)) && (ia_range_lcd == 0)))) {
      lcd_line_buffer[33] = (ascii[3] != '0') ? ascii[3] : ' ';
      lcd_line_buffer[34] = ascii[2]; lcd_line_buffer[35] = '.'; lcd_line_buffer[36] = ascii[1]; lcd_line_buffer[37] = ascii[0];
   } else {
      if (ascii[3] != '0') { lcd_line_buffer[33] = ascii[3]; lcd_line_buffer[34] = ascii[2]; }
      else { lcd_line_buffer[33] = ' '; lcd_line_buffer[34] = (ascii[2] != '0') ? ascii[2] : ' '; }
      lcd_line_buffer[35] = ascii[1]; lcd_line_buffer[36] = '.'; lcd_line_buffer[37] = ascii[0];
   }

   /* Ug2 */
   ug1_calc_accum = adc_mean_ug2;
   ug1_calc_accum *= adc_vref_scaled;
   ug1_calc_accum /= 107436;
   meas_ug2 = (uint16_t)ug1_calc_accum;
   if (sequencer_phase_ticks == (FUH+2)) ug1_calc_accum = lcd_ug2;
   if ((edit_field_index == 0) || (edit_field_index == 15)) {
      if (debounce_set_button_hold_ticks == DMAX) { ug1_calc_accum = catalog_current.voltage_screen_set; save_eeprom_flag = 1; }
      if ((debounce_encoder_ticks_stable_cnt == DMIN) && (edit_field_index == 15) && (save_eeprom_flag == 1)) {
         save_eeprom_flag = 0;
         if (tube_type_index == 0) setpoint_ug2 = catalog_current.voltage_screen_set;
         if (tube_type_index >= FLAMP) EEPROM_WRITE(&catalog_eeprom[tube_type_index-FLAMP].voltage_screen_set, catalog_current.voltage_screen_set);
      }
   }
   int2asc((uint16_t)ug1_calc_accum, ascii);
   if (ascii[2] != '0') { lcd_line_buffer[39] = ascii[2]; lcd_line_buffer[40] = ascii[1]; }
   else { lcd_line_buffer[39] = ' '; lcd_line_buffer[40] = (ascii[1] != '0') ? ascii[1] : ' '; }
   lcd_line_buffer[41] = ascii[0];

   /* Ig2 */
   ug1_calc_accum = adc_mean_ig2;
   ug1_calc_accum *= adc_vref_scaled;
   ug1_calc_accum >>= 13;
   ug1_calc_temp = adc_mean_ug2;
   ug1_calc_temp *= adc_vref_scaled;
   ug1_calc_temp *= 10;
   ug1_calc_temp /= 4369064;
   if (ug1_calc_accum > ug1_calc_temp) ug1_calc_accum -= ug1_calc_temp; else ug1_calc_accum = 0;
   meas_ig2 = (uint16_t)ug1_calc_accum;
   if (sequencer_phase_ticks == (FUH+2)) ug1_calc_accum = lcd_ig2;
   if ((edit_field_index == 0) || (edit_field_index == 16)) {
      if (debounce_set_button_hold_ticks == DMAX) { ug1_calc_accum = catalog_current.current_screen_ref; save_eeprom_flag = 1; }
      if ((debounce_encoder_ticks_stable_cnt == DMIN) && (edit_field_index == 16) && (save_eeprom_flag == 1)) {
         save_eeprom_flag = 0;
         if (tube_type_index >= FLAMP) EEPROM_WRITE(&catalog_eeprom[tube_type_index-FLAMP].current_screen_ref, catalog_current.current_screen_ref);
      }
   }
   int2asc((uint16_t)ug1_calc_accum, ascii);
   lcd_line_buffer[43] = (ascii[3] != '0') ? ascii[3] : ' ';
   lcd_line_buffer[44] = ascii[2]; lcd_line_buffer[45] = '.'; lcd_line_buffer[46] = ascii[1]; lcd_line_buffer[47] = ascii[0];

   /* S */
   ug1_calc_accum = slope_s;
   if (sequencer_phase_ticks == (FUH+2)) ug1_calc_accum = lcd_s;
   if ((edit_field_index == 0) || (edit_field_index == 17)) {
      if (debounce_set_button_hold_ticks == DMAX) { ug1_calc_accum = catalog_current.slope_ref; save_eeprom_flag = 1; }
      if ((debounce_encoder_ticks_stable_cnt == DMIN) && (edit_field_index == 17) && (save_eeprom_flag == 1)) {
         save_eeprom_flag = 0;
         if (tube_type_index >= FLAMP) EEPROM_WRITE(&catalog_eeprom[tube_type_index-FLAMP].slope_ref, catalog_current.slope_ref);
      }
   }
   int2asc(ug1_calc_accum, ascii);
   lcd_line_buffer[49] = (ascii[2] != '0') ? ascii[2] : ' ';
   lcd_line_buffer[50] = ascii[1]; lcd_line_buffer[51] = '.'; lcd_line_buffer[52] = ascii[0];

   /* R */
   ug1_calc_accum = resistance_r;
   if (sequencer_phase_ticks == (FUH+2)) ug1_calc_accum = lcd_r;
   if ((edit_field_index == 0) || (edit_field_index == 18)) {
      if (debounce_set_button_hold_ticks == DMAX) { ug1_calc_accum = catalog_current.r_anode_ref; save_eeprom_flag = 1; }
      if ((debounce_encoder_ticks_stable_cnt == DMIN) && (edit_field_index == 18) && (save_eeprom_flag == 1)) {
         save_eeprom_flag = 0;
         if (tube_type_index >= FLAMP) EEPROM_WRITE(&catalog_eeprom[tube_type_index-FLAMP].r_anode_ref, catalog_current.r_anode_ref);
      }
   }
   int2asc(ug1_calc_accum, ascii);
   lcd_line_buffer[54] = (ascii[2] != '0') ? ascii[2] : ' ';
   lcd_line_buffer[55] = ascii[1]; lcd_line_buffer[56] = '.'; lcd_line_buffer[57] = ascii[0];

   /* K */
   ug1_calc_accum = amplification_k;
   if (sequencer_phase_ticks == (FUH+2)) ug1_calc_accum = lcd_k;
   if ((edit_field_index == 0) || (edit_field_index == 19)) {
      if (debounce_set_button_hold_ticks == DMAX) { ug1_calc_accum = catalog_current.k_amplification_ref; save_eeprom_flag = 1; }
      if ((debounce_encoder_ticks_stable_cnt == DMIN) && (edit_field_index == 19) && (save_eeprom_flag == 1)) {
         save_eeprom_flag = 0;
         if (tube_type_index >= FLAMP) EEPROM_WRITE(&catalog_eeprom[tube_type_index-FLAMP].k_amplification_ref, catalog_current.k_amplification_ref);
      }
   }
   int2asc(ug1_calc_accum, ascii);
   if (ascii[3] != '0') { lcd_line_buffer[59] = ascii[3]; lcd_line_buffer[60] = ascii[2]; lcd_line_buffer[61] = ascii[1]; }
   else {
      lcd_line_buffer[59] = ' ';
      if (ascii[2] != '0') { lcd_line_buffer[60] = ascii[2]; lcd_line_buffer[61] = ascii[1]; }
      else { lcd_line_buffer[60] = ' '; lcd_line_buffer[61] = (ascii[1] != '0') ? ascii[1] : ' '; }
   }
   lcd_line_buffer[62] = ascii[0];

   /* Wyslanie pomiarow do PC */
   if (tx_send_flag) {
      EEPROM_WRITE(&eeprom_last_tube_type, tube_type_index);
      if (tube_type_index == 1) {
         for (i = 0; i < 10; i++) char2rs(legacy_rx_buffer[i]);
      } else {
         cstr2rs("\resistance_r\n");
         for (i = 0; i < 63; i++) {
            if (lcd_line_buffer[i] != '\0') char2rs(lcd_line_buffer[i]); else cstr2rs("  ");
         }
      }
      tx_send_flag = 0;
   }
}
