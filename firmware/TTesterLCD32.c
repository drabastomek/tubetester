//*************************************************************
//                           TESTER
//*************************************************************
// OCD  JTAG SPI CK  EE   BOOT BOOT BOOT BOD   BOD SU SU CK   CK   CK   CK
// EN    EN  EN  OPT SAVE SZ1  SZ0  RST  LEVEL EN  T1 T0 SEL3 SEL2 SEL1 SEL0
//  1     0   0   1   1    0    0    1    1     1   1  0  0    0    0    1 (0x99E1 DEFAULT)
//  1     1   0   0   1    0    0    1    0     0   0  1  1    1    1    1 (0xC91F)
//*************************************************************

#include <avr/io.h>
#include <avr/interrupt.h>
#include "config/config.h"
#include "protocol/vttester_remote.h"
#include "utils/utils.h"
#include "display/display.h"
#include "control/control.h"

uint8_t
   d, i,
   tx_send_flag,
   *edit_byte_ptr, edit_byte_min, edit_byte_max,
   edit_field_index, edit_field_min, edit_field_max,
   encoder_new_turn_flag,
   measurement_stop_request,
   sync_tick_divider,
   debounce_encoder_ticks_stable_cnt, debounce_set_button_hold_ticks,
   save_eeprom_flag, read_eeprom_flag,
   ia_range_hi, ia_range_lcd, ia_range_default,
   ascii[5],
   adc_channel_index,
   display_blink_phase,
   overcurrent_heater_count, overcurrent_anode_count, overcurrent_screen_count, alarm_error_bits, alarm_error_code,
   adc_phase_sample_count,
   heater_pwm_duty,
   anode_system_select,
   legacy_rx_buffer[10],
   lcd_line_buffer[64],
   proto_rx_frame[VTTESTER_FRAME_LEN],
   proto_tx_frame[VTTESTER_FRAME_LEN],
   remote_meas_trigger,
   remote_meas_pending,
   remote_meas_ready,
   remote_meas_index;

/* ISR ↔ main shared: must be volatile so optimizer does not assume they never change */
volatile uint8_t
   uart_tx_busy,       /* USART_TXC clears; main waits in char2rs */
   main_loop_sync_flag,       /* TIMER2_COMP sets; main checks in loop */
   delay_ticks_remaining,     /* TIMER2_COMP decrements; delay() spins */
   legacy_rx_index, legacy_rx_timeout_ticks, legacy_crc,
   proto_rx_pos,
   proto_frame_ready;

uint16_t
   *edit_value_ptr, edit_value_min, edit_value_max,
   sequencer_phase_ticks, heating_time_ticks,
   adc_vref_scaled,
   adc_raw_ih, adc_raw_ia, adc_raw_ig2,
   setpoint_uh,  setpoint_ih,  setpoint_ug1,  setpoint_ua,  setpoint_ia,  setpoint_ug2,   setpoint_ig2,
   adc_sum_uh, adc_sum_ih, adc_sum_ug1, adc_sum_ua, adc_sum_ia, adc_sum_ug2, adc_sum_ig2,
   adc_mean_uh, adc_mean_ih, adc_mean_ug1, adc_mean_ua, adc_mean_ia, adc_mean_ug2, adc_mean_ig2,
   ug1_dac_zero, ug1_dac_ref,
   meas_uh, meas_ih,
   meas_ua, meas_ua_left, meas_ua_right,
   meas_ia, meas_ia_left, meas_ia_right,
   meas_ug2, meas_ig2,
   meas_ug1, meas_ug1_left, meas_ug1_right,
   slope_s, resistance_r, amplification_k,
   tube_type_index, tube_type_index_prev,
   lcd_uh, lcd_ih, lcd_ug1, lcd_ua, lcd_ia, lcd_ug2, lcd_ig2, lcd_s, lcd_r, lcd_k,
   adc_sum_rez, adc_mean_rez,
   legacy_decoded_ua, legacy_decoded_ug2;

uint32_t
   calc_temp_a, calc_temp_b, ug1_calc_accum, ug1_calc_temp;

katalog
   catalog_remote,
   catalog_current;

//*************************************************************************
//                 O B S L U G A   P R Z E R W A N
//*************************************************************************

ISR(INT1_vect)
{
   if( sequencer_phase_ticks > (TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2) )
   {
      if( debounce_encoder_ticks_stable_cnt == DMIN )
      {
         sequencer_phase_ticks = (TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2);  // wylaczanie kreciolkiem
         measurement_stop_request = 0;
      }
   }
   else
   {
      if( sequencer_phase_ticks == (FUH+2) )
      {
         if( debounce_set_button_hold_ticks == DMAX )
         {
            if( RIGHT )
            {
               if( (encoder_new_turn_flag == 1) && ((catalog_current.tube_name[7] == '1') || (catalog_current.tube_name[7] == 28)) ) { tube_type_index++; encoder_new_turn_flag = 0; }
            }
            else
            {
               if( (encoder_new_turn_flag == 1) && ((catalog_current.tube_name[7] == '2') || (catalog_current.tube_name[7] == 29)) ) { tube_type_index--; encoder_new_turn_flag = 0; }
            }
            zersrk();
         }
         if( debounce_encoder_ticks_stable_cnt == DMIN )
         {
            sequencer_phase_ticks = (FUH+1);
         }
      }
      else
      {
         if( sequencer_phase_ticks == 0 )
         {
            if( edit_field_index == 0 )                          // edycja numeru
            {
               edit_value_min = 0;
               edit_value_max = (FLAMP+ELAMP-1); // PWRSUP+REMOTE+FLASH+EEPROM
               edit_value_ptr = &tube_type_index;
            }
            if( (edit_field_index > 0) && (edit_field_index < 7) )                 // edycja nazwy
            {
               edit_byte_min = 0;
               edit_byte_max = 36;
               edit_byte_ptr = &catalog_current.tube_name[edit_field_index-1];
            }
            if( edit_field_index == 7 )              // zmiana nr podstawki zarzenia
            {
               edit_byte_min = 0;
               edit_byte_max = 9;
               edit_byte_ptr = &catalog_current.tube_name[6];
            }
            if( edit_field_index == 8 )                // zmiana nr systemu elektrod
            {
               if( tube_type_index == 0 )
               {
                  edit_byte_min = 28;
               }
               else
               {
                  edit_byte_min = 27;
               }
               edit_byte_max = 29;
               edit_byte_ptr = &catalog_current.tube_name[7];
            }
            if( edit_field_index == 9 )                 // ustawianie czasu zarzenia
            {
               edit_byte_min = 28;
               edit_byte_max = 36;
               edit_byte_ptr = &catalog_current.tube_name[8];
            }
            if( edit_field_index == 10 )                         // ustawianie Ug1
            {
               edit_byte_min = (tube_type_index == 0)? 0: 5;          // 0.5 lub 0
               edit_byte_max = (tube_type_index == 0)? 240: 235;      // -23.5 lub -24.0V
               edit_byte_ptr = &catalog_current.voltage_g1_set;
            }
            if( edit_field_index == 11 )                         // ustawianie Uh
            {
               edit_byte_min = 0;
               edit_byte_max = 200;                           // 0..20.0V
               edit_byte_ptr = &catalog_current.voltage_heater_set;
            }
            if( edit_field_index == 12 )                         // ustawianie Ih
            {
               edit_byte_min = 0;
               edit_byte_max = 250;                     // 0..2.50A
               edit_byte_ptr = &catalog_current.current_heater_set;
            }
            if( edit_field_index == 13 )                         // ustawianie Ua
            {
               edit_value_min = (tube_type_index == 0)? 0: 10;
               edit_value_max = (tube_type_index == 0)? 300: 290;     // 0..300V
               edit_value_ptr = &catalog_current.voltage_anode_set;
            }
            if( edit_field_index == 14 )                         // ustawianie Ia
            {
               edit_value_min = 0;
               edit_value_max = 2000;                      // 0..200.0mA
               edit_value_ptr = &catalog_current.current_anode_ref;
            }
            if( edit_field_index == 15 )                        // ustawianie Ug2
            {
               edit_value_min = 0;
               edit_value_max = 300;                       // 0..300V
               edit_value_ptr = &catalog_current.voltage_screen_set;
            }
            if( edit_field_index == 16 )                         // ustawianie Ig2
            {
               edit_value_min = 0;
               edit_value_max = 4000;                      // 0..40.00mA
               edit_value_ptr = &catalog_current.current_screen_ref;
            }
            if( edit_field_index == 17 )                        // ustawianie S
            {
               edit_value_min = 0;
               edit_value_max = 999;                         // 99.9
               edit_value_ptr = &catalog_current.slope_ref;
            }
            if( edit_field_index == 18 )                        // ustawianie R
            {
               edit_value_min = 0;
               edit_value_max = 999;                      // 99.9
               edit_value_ptr = &catalog_current.r_anode_ref;
            }
            if( edit_field_index == 19 )                        // ustawianie K
            {
               edit_value_min = 0;
               edit_value_max = 9999;                       // 9999
               edit_value_ptr = &catalog_current.k_amplification_ref;
            }

            if( RIGHT )
            {
               if( (edit_field_index > 0) && (edit_field_index < 13) )
               {
                  if( debounce_set_button_hold_ticks == DMAX )
                  {
                     if( (*edit_byte_ptr) < edit_byte_max ) { (*edit_byte_ptr)++; }
                  }
               }
               else
               {
                  if( debounce_set_button_hold_ticks == DMAX )
                  {
                     if( (*edit_value_ptr) < edit_value_max ) { (*edit_value_ptr)++; } else { if( edit_field_index == 0 ) { (*edit_value_ptr) = 0; } }
                  }
               }
               if( debounce_encoder_ticks_stable_cnt == DMIN )
               {
                  if( tube_type_index == 0 ) // PWRSUP
                  {
                     if( edit_field_index < 15 ) { edit_field_index++; } // nie dojezdzaj do Ig2
                     if( edit_field_index == 14 ) { edit_field_index = 15; } // przeskocz Ia
                     if( edit_field_index == 9 ) { edit_field_index = 10; } // przeskocz czas zarzenia
                     if( edit_field_index == 1 ) { edit_field_index = 8; } // 8 przeskocz nazwe i podstawke
                  }
                  if( tube_type_index >= FLAMP ) // katalog zmienny
                  {
                     if( edit_field_index < 19 ) { edit_field_index++; }
                  }
               }
            }
            else
            {
               if( (edit_field_index > 0) && (edit_field_index < 13) )
               {
                  if( debounce_set_button_hold_ticks == DMAX )
                  {
                     if( (*edit_byte_ptr) > edit_byte_min ) { (*edit_byte_ptr)--; }
                  }
               }
               else
               {
                  if( debounce_set_button_hold_ticks == DMAX )
                  {
                     if( (*edit_value_ptr) > edit_value_min ) { (*edit_value_ptr)--; } else { if( edit_field_index == 0 ) { (*edit_value_ptr) = (FLAMP+ELAMP-1); } }
                  }
               }
               if( (debounce_encoder_ticks_stable_cnt == DMIN) && (edit_field_index > 0) )
               {
                  if( tube_type_index >= FLAMP ) { edit_field_index--; }
                  if( tube_type_index == 0 )
                  {
                     if( edit_field_index > 0 ) { edit_field_index--; }
                     if( edit_field_index == 7 ) { edit_field_index = 0; measurement_stop_request = 0; sequencer_phase_ticks = (TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2); } // przeskocz nazwe / wylaczenie
                     if( edit_field_index == 9 ) { edit_field_index = 8; } // przeskocz nazwe i podstawke
                     if( edit_field_index == 14 ) { edit_field_index = 13; } // pomin Ia
                  }
               }
            }
         }
      }
   }
}

ISR(ADC_vect)
{
   switch( adc_channel_index )
   {
      case 0:
      {
         adc_sum_rez += ADC;
         adc_channel_index = 1;
         CLKUG1SET;
         ADMUX = ADRIH;
         break;
      }
      case 1:
      {
         adc_channel_index = 2;
         if( ADC >= setpoint_ug1 ) { CLKUG1RST; }
         ADMUX = ADRUG1;
         break;
      }
      case 2:
      {
         adc_raw_ih = ADC;
         adc_sum_ih += adc_raw_ih;
         adc_channel_index = 3;
         CLKUG1SET;
         ADMUX = ADRUH;
//***** Zabezpieczenie nadpradowe Ih **************************
         if( adc_raw_ih > 400 )
         {
            if( overcurrent_heater_count > 0 )
            {
               overcurrent_heater_count--;
            }
            else
            {
               setpoint_uh = setpoint_ih = 0;
               alarm_error_bits |= OVERIH;
            }
         }
         else
         {
            overcurrent_heater_count = OVERSAMP;
         }
         break;
      }
      case 3:
      {
         adc_channel_index = 4;
         if( ADC >= setpoint_ug1 ) { CLKUG1RST; }
         ADMUX = ADRUG1;
         break;
      }
      case 4:
      {
         adc_sum_uh += ADC;
         adc_channel_index = 5;
         CLKUG1SET;
         ADMUX = ADRUA;
         break;
      }
      case 5:
      {
         adc_channel_index = 6;
         if( ADC >= setpoint_ug1 ) { CLKUG1RST; }
         ADMUX = ADRUG1;
         break;
      }
      case 6:
      {
         adc_sum_ua += ADC;
         adc_channel_index = 7;
         CLKUG1SET;
         ADMUX = ADRIA;
         break;
      }
      case 7:
      {
         adc_channel_index = 8;
         if( ADC >= setpoint_ug1 ) { CLKUG1RST; }
         ADMUX = ADRUG1;
         break;
      }
      case 8:
      {
         adc_raw_ia = ADC;
         adc_sum_ia += adc_raw_ia;
         adc_channel_index = 9;
         CLKUG1SET;
         ADMUX = ADRUG2;
//***** Zabezpieczenie nadpradowe Ia ***************************
         if( (ia_range_hi != 0) && (adc_raw_ia >= 1020) )
         {
            if( overcurrent_anode_count > 0 )
            {
               overcurrent_anode_count--;
            }
            else
            {
               setpoint_ua = setpoint_ug2 = 0;
               PWMUA = PWMUG2 = 0;;
               alarm_error_bits |= OVERIA;
            }
         }
         else
         {
            overcurrent_anode_count = OVERSAMP;
         }
//***** Wystawienie Ua ****************************************
         if( PWMUA < setpoint_ua ) PWMUA++;
         if( PWMUA > setpoint_ua ) PWMUA--;
//***** Ustawianie wysokiego zakresu pomiarowego Ia ***********
         if( (ia_range_hi == 0) && (adc_raw_ia > 950) )
         {
            ia_range_hi = 1;
            SET200;
         }
//***** Ustawianie niskiego zakresu pomiarowego Ia ************
         if( ((alarm_error_bits & OVERIA) == 0) && (ia_range_hi != 0) && (adc_raw_ia < 85) )
         {
            ia_range_hi = 0;
            RST200;
         }
         break;
      }
      case 9:
      {
         adc_channel_index = 10;
         if( ADC >= setpoint_ug1 ) { CLKUG1RST; }
         ADMUX = ADRUG1;
         break;
      }
      case 10:
      {
         adc_sum_ug2 += ADC;
         adc_channel_index = 11;
         CLKUG1SET;
         ADMUX = ADRIG2;
         break;
      }
      case 11:
      {
         adc_channel_index = 12;
         if( ADC >= setpoint_ug1 ) { CLKUG1RST; }
         ADMUX = ADRUG1;
         break;
      }
      case 12:
      {
         adc_raw_ig2 = ADC;
         adc_sum_ig2 += adc_raw_ig2;
         adc_channel_index = 13;
         CLKUG1SET;
         ADMUX = ADRREZ;
//***** Zabezpieczenie nadpradowe Ig2 **************************
         if( adc_raw_ig2 >= 1020 )
         {
            if( overcurrent_screen_count > 0 )
            {
               overcurrent_screen_count--;
            }
            else
            {
               setpoint_ug2 = 0;
               PWMUG2 = 0;
               alarm_error_bits |= OVERIG;
            }
         }
         else
         {
            overcurrent_screen_count = OVERSAMP;
         }
//***** Wystawienie Ug2 ***************************************
         if( PWMUG2 < setpoint_ug2 ) PWMUG2++;
         if( PWMUG2 > setpoint_ug2 ) PWMUG2--;
         break;
      }
      case 13:
      {
         adc_sum_ug1 += ADC;
         adc_channel_index = 0;
         if( ADC >= setpoint_ug1 ) { CLKUG1RST; }
         ADMUX = ADRUG1;
         if( adc_phase_sample_count == LPROB )
         {
            adc_mean_rez = adc_sum_rez;
            adc_mean_ih = adc_sum_ih;
            adc_mean_uh = adc_sum_uh;
            adc_mean_ua = adc_sum_ua;
            adc_mean_ia = adc_sum_ia;
            adc_mean_ug2 = adc_sum_ug2;
            adc_mean_ig2 = adc_sum_ig2;
            adc_mean_ug1 = adc_sum_ug1;
            adc_sum_rez = adc_sum_ih = adc_sum_uh = adc_sum_ua = adc_sum_ia = adc_sum_ug2 = adc_sum_ig2 = adc_sum_ug1 = 0;
            adc_phase_sample_count = 0;
//***** Szybkie wylaczenie Uh *********************************
            if( (setpoint_uh == 0) && (setpoint_ih == 0) )
               heater_pwm_duty = 0;
            else
            {
               TCCR0 |= BIT(COM01);               // dolacz OC0
//***** Stabilizacja Uh ***************************************
               if( setpoint_uh > 0 )
               {
                  calc_temp_a = adc_mean_uh;
                  calc_temp_a *= adc_vref_scaled;
                  calc_temp_a >>= 14;        //  /= 16384;                // 0..200
                  calc_temp_b = adc_mean_ih;   // poprawka na spadek napiecia na boczniku
                  calc_temp_b *= adc_vref_scaled;
                  calc_temp_b >>= 16;        //  /= 65536;
                  if( calc_temp_a > calc_temp_b ) { calc_temp_a -= calc_temp_b; } else { calc_temp_a = 0; }
                  calc_temp_a /= 10;
                  if( (setpoint_uh > (uint16_t)calc_temp_a) && (heater_pwm_duty < 255) ) { heater_pwm_duty++; }
                  if( (setpoint_uh < (uint16_t)calc_temp_a) && (heater_pwm_duty >   0) ) { heater_pwm_duty--; }
               }
//***** Stabilizacja Ih ***************************************
               if( setpoint_ih > 0 )
               {
                  calc_temp_a = adc_mean_ih;
                  calc_temp_a *= adc_vref_scaled;
                  calc_temp_a >>= 15;    //   /= 32768;
                  if( (setpoint_ih > (uint16_t)calc_temp_a) )
                  {
                     if( (heater_pwm_duty < 8) || ((adc_mean_ih > 32) && (heater_pwm_duty < 255)) ) { heater_pwm_duty++; } // Uh<0.5V lub Ih>5mA
                  }
                  if( (setpoint_ih < (uint16_t)calc_temp_a) && (heater_pwm_duty >   0) ) { heater_pwm_duty--; }
               }
            }
            if( heater_pwm_duty == 0 )
            {
               TCCR0 &= ~BIT(COM01);                        // odlacz OC0
            }
            else
            {
               TCCR0 |= BIT(COM01);                          // dolacz OC0
            }
            PWMUH = heater_pwm_duty;
//***** Ustawianie/kasowanie znacznika przegrzania ************
            if( adc_mean_rez > HITEMP ) alarm_error_bits |= OVERTE;
            if( adc_mean_rez < LOTEMP ) alarm_error_bits &= ~OVERTE;
         }
         else
         {
            adc_phase_sample_count++;
         }
         break;
      }
   }
// NOP;
}

ISR(USART_TXC_vect)
{
   uart_tx_busy = 0;
}

ISR(USART_RXC_vect)
{
   /* Feed byte to VTTester 8-byte protocol buffer */
   proto_rx_frame[proto_rx_pos++] = UDR;
   if( proto_rx_pos >= VTTESTER_FRAME_LEN ) { proto_rx_pos = 0; proto_frame_ready = 1; }
   /* 10-byte legacy path: remote (tube_type_index==1) only. Power supply (tube_type_index==0) uses 8-byte protocol only. */
   if( tube_type_index == 1 )
   {
      legacy_rx_buffer[legacy_rx_index] = UDR;
      if( legacy_rx_index < 9 )
      {
         legacy_rx_index++;
      }
      else
      {
         legacy_decoded_ua = legacy_rx_buffer[6]; legacy_decoded_ua <<= 8; legacy_decoded_ua += legacy_rx_buffer[5];
         legacy_decoded_ug2 = legacy_rx_buffer[8]; legacy_decoded_ug2 <<= 8; legacy_decoded_ug2 += legacy_rx_buffer[7];

         if( (((uint16_t)(legacy_rx_buffer[6])<<8) + legacy_rx_buffer[5]) > 300 ) NOP;

         /* 10-byte legacy path: no CRC (backward-compatible LCD dump); byte 9 unused */
         if( !((legacy_rx_buffer[0] != ESC)||
              ((legacy_rx_buffer[1] != 28)&&(legacy_rx_buffer[1] != 29))||
               (legacy_rx_buffer[2] > 240)||
               (legacy_rx_buffer[3] > 200)||
               (legacy_rx_buffer[4] > 250)||
              ((legacy_rx_buffer[3] !=0 ) && (legacy_rx_buffer[4] !=0 ))||
               (legacy_decoded_ua > 300)||
               (legacy_decoded_ug2 > 300 )) )
         {
            catalog_remote.tube_name[7] = legacy_rx_buffer[1];
            setpoint_ug1 = liczug1( catalog_remote.voltage_g1_set = legacy_rx_buffer[2] );
            setpoint_uh = catalog_remote.voltage_heater_set = legacy_rx_buffer[3];
            setpoint_ih = catalog_remote.current_heater_set = legacy_rx_buffer[4];
            setpoint_ua = catalog_remote.voltage_anode_set = legacy_decoded_ua;
            setpoint_ug2 = catalog_remote.voltage_screen_set = legacy_decoded_ug2;
         }
         legacy_rx_index = 0;
         tx_send_flag = 1;
      }
      legacy_rx_timeout_ticks = MS250; // zeruj odliczanie czasu od ostatniego
   }
   else
   {
      if( UDR == ESC ) { tx_send_flag = 1; }
   }
}

ISR(TIMER2_COMP_vect)
{
   if( delay_ticks_remaining != 0 ) { delay_ticks_remaining--; }
   if( legacy_rx_timeout_ticks > 1 ) { legacy_rx_timeout_ticks--; }
   if( legacy_rx_timeout_ticks == 1 ) { legacy_rx_timeout_ticks = 0; legacy_rx_index = 0; legacy_crc = 0; }
   if( anode_system_select == 29 ) SETSEL; else RSTSEL;

   if( DUSK0 )
   {
      if( debounce_set_button_hold_ticks < DMAX ) { debounce_set_button_hold_ticks++; } else { debounce_encoder_ticks_stable_cnt = 0; }
   }
   else
   {
      if( debounce_encoder_ticks_stable_cnt < DMIN )
      {
         debounce_encoder_ticks_stable_cnt++;
      }
      else
      {
         if( (debounce_set_button_hold_ticks > DMIN) && (debounce_set_button_hold_ticks < DMAX) )
         {
            if( (tube_type_index > 1) && (alarm_error_bits == 0) )
            {
               if( (sequencer_phase_ticks == 0) && (edit_field_index == 0) )   // sequencer_phase_ticks
               {
                  sequencer_phase_ticks = (heating_time_ticks+(TUG+TUA+TUG2+TMAR+TUG+TMAR+TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2));
                  measurement_stop_request = 1;                 // measurement_stop_request po zakonczeniu pomiaru
                  zersrk();
               }
               if( sequencer_phase_ticks == (FUH+2) ) // restart
               {
                  sequencer_phase_ticks = (TUG+TUA+TUG2+TMAR+TUG+TMAR+TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2); // ponowny pomiar
                  measurement_stop_request = 1;                    // measurement_stop_request po zakonczeniu pomiaru
                  zersrk();
               }
            }
         }
         debounce_set_button_hold_ticks = 0;
      }
   }
   sync_tick_divider++;
   if( sync_tick_divider >= MRUG )
   {
      sync_tick_divider = 0;
      main_loop_sync_flag = 1;
      if( remote_meas_trigger && (sequencer_phase_ticks == 0) && (alarm_error_bits == 0) && (tube_type_index == 1) )
      {
         sequencer_phase_ticks = (heating_time_ticks+(TUG+TUA+TUG2+TMAR+TUG+TMAR+TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2));
         measurement_stop_request = 1;
         zersrk();
         remote_meas_trigger = 0;
      }
//***** Sekwencja zalaczanie/wylaczanie napiec ****************
      if( sequencer_phase_ticks == (heating_time_ticks+(TUG+TUA+TUG2+TMAR+TUG+TMAR+TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) )
      {
         if( catalog_current.voltage_heater_set != 0)
         {
            setpoint_uh = catalog_current.voltage_heater_set;
            setpoint_ih = catalog_current.current_heater_set = 0;
         }
         if( catalog_current.current_heater_set != 0)
         {
            setpoint_ih = catalog_current.current_heater_set;
            setpoint_uh = catalog_current.voltage_heater_set = 0;
         }
      }
      if( sequencer_phase_ticks == (    (TUG+TUA+TUG2+TMAR+TUG+TMAR+TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) )
      {
         if( (catalog_current.tube_name[7] == '2') || (catalog_current.tube_name[7] == 29) ) { anode_system_select = 29; } // anoda 2
         setpoint_ug1 = ug1_dac_ref - 11; // UgR
      }
      if( sequencer_phase_ticks == (    (    TUA+TUG2+TMAR+TUG+TMAR+TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) )
      {
         setpoint_ua = catalog_current.voltage_anode_set;
      }
      if( sequencer_phase_ticks == (    (        TUG2+TMAR+TUG+TMAR+TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { setpoint_ug2 = catalog_current.voltage_screen_set; }

// wystaw Uh/Ih   UgR   Ua   Ug2            UgL          Ug   UaL           UaR          Ua              Ug2    Ua   Ug   Uh/Ih   SPKON     [SPKOFF]     SPKON      SPKOFF STOP Tx
// czekaj      heating_time_ticks   TUG  TUA   TUG2    TMAR   TUG   TMAR  TUG   TUA    TMAR   TUA   TMAR  TUA       TMAR   FUG2  FUA  FUG     FUH     BEEP-0      BEEP-1     BEEP-2      2    1  0
// read_eeprom_flag                           IagR          IagL              IaaL          IaaR        Ug2/Ig2
// oblicz                                          S                               R      K

      if( sequencer_phase_ticks == (    (             TMAR+TUG+TMAR+TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { meas_ug1_right = meas_ug1; meas_ia_right = (ia_range_hi==0)?meas_ia:meas_ia*10; } // IagR
      if( sequencer_phase_ticks == (    (                  TUG+TMAR+TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { setpoint_ug1 = ug1_dac_ref + 11; } // UgL
      if( sequencer_phase_ticks == (    (                      TMAR+TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { meas_ug1_left = meas_ug1; meas_ia_left = (ia_range_hi==0)?meas_ia:meas_ia*10; if( meas_ia_right != meas_ia_left ) { meas_ia_left -= meas_ia_right; meas_ug1_right -= meas_ug1_left; slope_s = meas_ia_left; slope_s /= meas_ug1_right; } else slope_s = 999; } // IagL S
      if( sequencer_phase_ticks == (    (                           TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { setpoint_ug1 = ug1_dac_ref; } // Ug
      if( sequencer_phase_ticks == (    (                               TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { setpoint_ua = catalog_current.voltage_anode_set - 10; } // UaL
      if( sequencer_phase_ticks == (    (                                   TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { meas_ua_left = meas_ua; meas_ia_left = (ia_range_hi==0)?meas_ia:meas_ia*10; } // IaaL
      if( sequencer_phase_ticks == (    (                                        TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { setpoint_ua = catalog_current.voltage_anode_set + 10; } // UaR
      if( sequencer_phase_ticks == (    (                                            TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { meas_ua_right = meas_ua; meas_ia_right = (ia_range_hi==0)?meas_ia:meas_ia*10; if( meas_ia_right != meas_ia_left ) { meas_ua_right -= meas_ua_left; meas_ua_right *= 1000; meas_ia_right -= meas_ia_left; resistance_r = meas_ua_right; resistance_r /= meas_ia_right; } else resistance_r = 999; } // IaaR R
      if( sequencer_phase_ticks == (    (                                                 TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { setpoint_ua = catalog_current.voltage_anode_set; calc_temp_a = slope_s; calc_temp_a *= resistance_r; calc_temp_a += 50; calc_temp_a /= 100; if( calc_temp_a < 9999 ) { amplification_k = (uint16_t)calc_temp_a; } else amplification_k = 9999; } // K=R*S
      if( sequencer_phase_ticks == (    (                                                     TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { lcd_uh = meas_uh; lcd_ih = meas_ih; lcd_ug1 = meas_ug1; lcd_ua = meas_ua; lcd_ia = meas_ia; ia_range_lcd = ia_range_hi; lcd_ug2 = meas_ug2; lcd_ig2 = meas_ig2; lcd_s = slope_s; lcd_r = resistance_r; lcd_k = amplification_k; tx_send_flag = 1; if( remote_meas_pending ) remote_meas_ready = 1; }
      if( sequencer_phase_ticks == (    (                                                          FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { setpoint_ug2 = 0; if( tube_type_index == 0 ) catalog_current.voltage_screen_set = 0; }
      if( sequencer_phase_ticks == (    (                                                               FUA+FUG+(BIP-0)+FUH+2)) ) { setpoint_ua = 0; if( tube_type_index == 0 ) catalog_current.voltage_anode_set = 0; }
      if( sequencer_phase_ticks == (    (                                                                   FUG+(BIP-0)+FUH+2)) ) { setpoint_ug1 = ug1_dac_zero; anode_system_select = 28; }
      if( sequencer_phase_ticks == (    (                                                                       (BIP-0)+FUH+2)) ) { if( measurement_stop_request != 0 ) SPKON; }
      if( sequencer_phase_ticks == (    (                                                                       (BIP-1)+FUH+2)) ) { if( alarm_error_bits != 0 ) { SPKOFF; setpoint_uh = setpoint_ih = 0; if( tube_type_index == 0 ) { catalog_current.voltage_heater_set = catalog_current.current_heater_set = 0; } } } // alarm - wylacz tez zarzenie
      if( sequencer_phase_ticks == (    (                                                                       (BIP-2)+FUH+2)) ) { if( measurement_stop_request != 0 ) SPKON; }
      if( sequencer_phase_ticks == (    (                                                                       (BIP-3)+FUH+2)) ) { SPKOFF; }
      if( sequencer_phase_ticks == (    (                                                                               FUH+1)) ) { setpoint_uh = setpoint_ih = 0; if( tube_type_index == 0 ) { catalog_current.voltage_heater_set = catalog_current.current_heater_set = 0; } }
      if( sequencer_phase_ticks == (    (                                                                                   1)) ) { alarm_error_bits = 0; zersrk(); }

      display_blink_phase++;
      display_blink_phase &= 0x03;
      if( (display_blink_phase == 2) && (debounce_set_button_hold_ticks == DMAX) ) display_blink_phase = 0;

      if( sequencer_phase_ticks == (FUH+2) )
      {
         if( measurement_stop_request == 0 ) sequencer_phase_ticks = (FUH+1);
      }
      else
      {
         if( sequencer_phase_ticks > 0 ) { sequencer_phase_ticks--; }
      }
   }
}

//*************************************************************
//      Program glowny
//*************************************************************

int main(void)
{
//***** Konfiguracja portow ***********************************
                            //   7   6   5   4   3   2   1   0
                            // UG1 IG2 UG2  IA  UA  UH  IH REZ
   DDRA  = 0x00;            //   0   0   0   0   0   0   0   0
   PORTA = 0x00;            //   0   0   0   0   0   0   0   0
                            //  D7  D6  D5  K1  UH CKG SEL RNG
   DDRB  = 0x0f;            //   0   0   0   0   1   1   1   1
   PORTB = 0xf0;            //   1   1   1   1   0   0   0   0
                            //  D7  D6  D5  D4  ENA RS SDA SCL
   DDRC  = 0xfc;            //   1   1   1   1   1   1   0   0
   PORTC = 0x03;            //   0   0   0   0   0   0   1   1
                            // SPK DIR UG2  UA CLK  K0 TXD RXD
   DDRD  = 0xb2;            //   1   0   1   1   0   0   1   0
   PORTD = 0x4f;            //   0   1   0   0   1   1   1   1

//***** Konfiguracja procesora ********************************

   ACSR = BIT(ACD);                       // wylacz komparator

//***** Konfiguracja timerow **********************************

   TCCR2 = BIT(FOC2)|BIT(WGM21)|BIT(CS22);  // FOC2/CTC,XTAL/64
   OCR2 = MS1;                                           // 1ms

//***** Konfiguracja wyjsc PWM ********************************

   TCCR0 = BIT(WGM01)|BIT(WGM00)|BIT(CS00); // FAST,XTAL, OC0 odlaczone

   TCCR1A = BIT(COM1A1)|BIT(COM1B1);                    // PWM
   TCCR1B = BIT(WGM13)|BIT(CS10);      // PHASE+FREQ,ICR1,XTAL

//***** Konfiguracja przetwornika ADC *************************

   ADMUX = ADRUG1;
   ADCSRA = BIT(ADEN)|BIT(ADSC)|BIT(ADATE)|BIT(ADIF)|BIT(ADIE)|BIT(ADPS2)|BIT(ADPS1)|BIT(ADPS0);

//***** Konfiguracja portu szeregowego ************************

   UBRRL = RATE;                             // ustaw szybkosc
   UCSRB = BIT(RXCIE)|BIT(RXEN)|BIT(TXCIE)|BIT(TXEN);
   UCSRC = BIT(URSEL)|BIT(UCSZ1)|BIT(UCSZ0);

//***** Inicjalizacja watchdoga *******************************

   WDTCR = BIT(WDE);
   WDTCR = BIT(WDE)|BIT(WDP2)|BIT(WDP1);
   WDR;

//***** Inicjalizacja przerwan ********************************

   MCUCR = BIT(ISC11);                       // INT1 na zboczu
   TIMSK = BIT(OCIE2);                               // T2 CTC
   GIFR = BIT(INTF1);                  // skasuj znacznik INT1

//***** Inicjalizacja zmiennych *******************************

   adc_vref_scaled = 509;                                  // adc_vref_scaled >= 480
   TOPPWM = 61 * adc_vref_scaled / 100;             // okres PWM Ua i Ug2

   setpoint_ug1 = ug1_dac_zero = ug1_dac_ref = liczug1( 240 );    // -24.0V
   catalog_current.voltage_g1_set = 240;
   anode_system_select = 28;
   legacy_rx_index = 0;
   proto_rx_pos = 0;
   proto_frame_ready = 0;
   remote_meas_trigger = 0;
   remote_meas_pending = 0;
   remote_meas_ready = 0;
   legacy_rx_timeout_ticks = MS250; // zeruj odliczanie czasu od ostatniego rx
   load_lamprom( 1, &catalog_remote ); // load from EEPROM rather than hold in RAM
   edit_value_min = 0;
   edit_value_max = (ELAMP+FLAMP-1);  // wszystkie lampy
   edit_value_ptr = &tube_type_index;                               // wskaz na Typ
   EEPROM_READ(&eeprom_last_tube_type, tube_type_index); // ustaw na ostatnio aktywny
   tube_type_index_prev = tube_type_index;
   ADMUX = ADRIH;

   SEI;                                    // wlacz przerwania

//***** Inicjalizacja wyswietlacza LCD ************************
   display_init();
   for( i = 0; i < 20; i++ ) { WDR; if( i == 19 ) { SPKON; } delay( 100 ); SPKOFF; };

// 01234567890123456789
// ====================
// 001 ECC83_J29 G-24.0
// H=12.6V A=300 G2=300
// T 2550mA 199.9 39.99
// S=99.9 u=99.9 R=9999
// ====================

   cstr2rs( "\resistance_r\nPress <ESC> to get LCD copy\resistance_r\nNr Type Uh[V] Ih[mA] -Ug[V] Ua[V] Ia[mA] Ug2[V] Ig2[mA] S[mA/V] R[amplification_k] K[V/V]" );

//***** Glowna petla programu *********************************

   while( 1 )
   {
      WDR;
      if ( main_loop_sync_flag == 1 )
      {
         main_loop_sync_flag = 0;

//***** VTTester 8-byte protocol (power supply tube_type_index==0, remote tube_type_index==1 only) ***
         /* Parser sets parsed.err_code (OK, OUT_OF_RANGE, CRC, INVALID_CMD, UNKNOWN); we send it + error_param/error_value for SET. */
         if( tube_type_index <= 1 )
         {
            if( proto_frame_ready )
            {
               vttester_parsed_t parsed;
               uint8_t cmd, i;
               proto_frame_ready = 0;
               cmd = vttester_parse_message( proto_rx_frame, &parsed );
               // if command is SET, check if it'slope_s valid and send ACK or ERR
               if( cmd == VTTESTER_CMD_SET )
               {
                  if( parsed.set.error_param == 0 && tube_type_index == 1 )
                  {
                     catalog_remote.voltage_heater_set = parsed.set.voltage_heater_set;
                     catalog_remote.current_heater_set = parsed.set.current_heater_set;
                     catalog_remote.voltage_g1_set = parsed.set.voltage_g1_set;
                     catalog_remote.voltage_anode_set = parsed.set.voltage_anode_set;
                     catalog_remote.voltage_screen_set = parsed.set.voltage_screen_set;
                     setpoint_uh = parsed.set.voltage_heater_set;
                     setpoint_ih = parsed.set.current_heater_set;
                     setpoint_ug1 = ug1_dac_ref = liczug1( parsed.set.voltage_g1_set );
                     setpoint_ua = parsed.set.voltage_anode_set;
                     setpoint_ug2 = parsed.set.voltage_screen_set;
                     heating_time_ticks = (uint16_t)parsed.set.tuh_ticks * 240u / 60u;
                     catalog_current = catalog_remote;
                  }
                  // send ACK or ERR
                  vttester_send_response( proto_tx_frame, parsed.index,
                     parsed.err_code, parsed.set.error_param, parsed.set.error_value );
                  for( i = 0; i < VTTESTER_FRAME_LEN; i++ ) char2rs( proto_tx_frame[i] );
               }
               // if command is MEAS, send OK and sequencer_phase_ticks measurement
               else if( cmd == VTTESTER_CMD_MEAS )
               {
                  vttester_send_response( proto_tx_frame, parsed.index, VTTESTER_ERR_OK, 0, 0 );
                  for( i = 0; i < VTTESTER_FRAME_LEN; i++ ) char2rs( proto_tx_frame[i] );
                  remote_meas_index = parsed.index;
                  remote_meas_pending = 1;
                  remote_meas_trigger = 1;
               }
               else
               {
                  /* cmd == VTTESTER_CMD_NONE: CRC, invalid cmd, or unknown */
                  vttester_send_response( proto_tx_frame, parsed.index, parsed.err_code, 0, 0 );
                  for( i = 0; i < VTTESTER_FRAME_LEN; i++ ) char2rs( proto_tx_frame[i] );
               }
            }
            // if measurement is ready, send the result
            if( remote_meas_ready )
            {
               uint8_t alarm_bits = 0, i;
               if( alarm_error_bits & OVERIH ) alarm_bits |= VTTESTER_ALARM_OVERIH;
               if( alarm_error_bits & OVERIA ) alarm_bits |= VTTESTER_ALARM_OVERIA;
               if( alarm_error_bits & OVERIG ) alarm_bits |= VTTESTER_ALARM_OVERIG;
               if( alarm_error_bits & OVERTE ) alarm_bits |= VTTESTER_ALARM_OVERTE;
               vttester_send_measurement( proto_tx_frame, remote_meas_index, lcd_ih, lcd_ia, ia_range_lcd, lcd_ig2, lcd_s, alarm_bits );
               for( i = 0; i < VTTESTER_FRAME_LEN; i++ ) char2rs( proto_tx_frame[i] );
               remote_meas_ready = 0;
               remote_meas_pending = 0;
            }
         }
         else
         {
            /* Local mode (tube_type_index > 1): ignore protocol; discard any received 8-byte frame */
            if( proto_frame_ready ) proto_frame_ready = 0;
         }

//***** Wyswietlenie zawartosci bufora ************************
         display_refresh();
      }
      GICR = BIT(INT1);                             // wlacz INT1
      if( (alarm_error_bits != 0) && ((sequencer_phase_ticks == 0) || (sequencer_phase_ticks > (TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2))) ) { measurement_stop_request = 1; sequencer_phase_ticks = (TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2); } // awaryjne wylaczenie

//***** Menu, katalog, edycja, obliczenia (wypelnianie lcd_line_buffer[]) *****
      control_update( ascii );
   }

   return 0;
}
