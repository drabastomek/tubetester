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
#include <avr/eeprom.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include "config/config.h"
#include "utils/utils.h"
#include "display/display.h"
#include "interrupts/interrupts.h"
#include "communication/communication.h"

// // MINIMAL globals here
// uint8_t
//    ascii[5],
//    bufin[10],
//    buf[64];

// static const uint8_t CRC8TABLE[256] =
// {
// 	  0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
// 	157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
// 	 35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
// 	190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
// 	 70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
// 	219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
// 	101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
// 	248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
// 	140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
// 	 17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
// 	175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
// 	 50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
// 	202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
// 	 87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
// 	233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
// 	116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53
// };


//*************************************************************************
//                 F U N K C J E  P O M O C N I C Z E
//*************************************************************************

// void char2rs(uint8_t bajt)
// {
//    while( !(UCSRA & (1 << UDRE)) ) ;   /* wait for transmit buffer empty (poll; works without interrupts) */
//    UDR = bajt;
// }

// void cstr2rs( const char* q )
// {
//    while( *q )                             // do konca stringu
//    {
//       while( !(UCSRA & (1 << UDRE)) ) ;   /* wait for transmit buffer empty (poll; works without interrupts) */
//       UDR = *q;
//       q++;
//    }
// }


//*************************************************************************
//                 O B S L U G A   P R Z E R W A N
//*************************************************************************

/* Event ring: ISRs push event bits, main drains and dispatches */
static volatile uint8_t  event_ring[EVENT_RING_SIZE];
static volatile uint8_t  event_head = 0, event_tail = 0;


static inline void event_push(uint8_t ev)
{
   uint8_t next = (event_head + 1) % EVENT_RING_SIZE;
   if (next != event_tail)
   {
      event_ring[event_head] = ev;
      event_head = next;
   }
}

ISR(INT1_vect) { event_push(EVENT_INT1); }
ISR(USART_TXC_vect) { event_push(EVENT_UART_TXC); }
ISR(TIMER2_COMP_vect) { event_push(EVENT_TIMER2); }

ISR(USART_RXC_vect) {
   uint8_t next = (uart_rx_head + 1) % UART_RX_RING_SIZE;
   if ( next != uart_rx_tail )
   {
      uart_rx_ring[uart_rx_head] = UDR;
      uart_rx_head = next;
      event_push(EVENT_UART_RXC);
   }
}


/*
 * ADC round-robin (14 slots, channel 0..13)
 * -----------------------------------------
 * The ADC runs in auto-trigger mode: each conversion completion fires ADC_vect.
 * When we enter the ISR with channel=K, the value in ADC is the result of the
 * conversion that just finished. That conversion was for the MUX we set when
 * we last left the ISR (i.e. when channel was K-1, or 13 when K=0). So the
 * result is always one slot behind: we are now processing the channel we
 * selected in the previous iteration.
 *
 * This ISR only (1) captures ADC and channel for the handler, (2) advances
 * the round-robin by setting the next ADMUX and channel, and (3) pushes
 * EVENT_ADC. All accumulation, limits, and PWM updates run in adc_handler()
 * in the main loop (interrupts.c).
 *
 * Slot (channel) → physical input we set when leaving that slot:
 *   0→IH  1→UG1  2→UH  3→UG1  4→UA  5→UG1  6→IA  7→UG1
 *   8→UG2 9→UG1 10→IG2 11→UG1 12→REZ 13→UG1
 * So the result we get when entering with channel=K is from the slot above
 * for K-1 (or 13 when K=0). UG1 is sampled every other slot for the ug1set
 * comparison and CLKUG1 toggling; the others are the main signals for
 * accumulation and control.
 */
ISR(ADC_vect)
{
   adc_result = ADC;
   adc_channel = channel;

   switch ( channel )
   {
      case  0: channel =  1; CLKUG1SET; ADMUX = ADRIH;   break;
      case  1: channel =  2; if ( adc_result >= ug1set ) CLKUG1RST; ADMUX = ADRUG1; break;
      case  2: channel =  3; CLKUG1SET; ADMUX = ADRUH;   break;
      case  3: channel =  4; if ( adc_result >= ug1set ) CLKUG1RST; ADMUX = ADRUG1; break;
      case  4: channel =  5; CLKUG1SET; ADMUX = ADRUA;   break;
      case  5: channel =  6; if ( adc_result >= ug1set ) CLKUG1RST; ADMUX = ADRUG1; break;
      case  6: channel =  7; CLKUG1SET; ADMUX = ADRIA;   break;
      case  7: channel =  8; if ( adc_result >= ug1set ) CLKUG1RST; ADMUX = ADRUG1; break;
      case  8: channel =  9; CLKUG1SET; ADMUX = ADRUG2;  break;
      case  9: channel = 10; if ( adc_result >= ug1set ) CLKUG1RST; ADMUX = ADRUG1; break;
      case 10: channel = 11; CLKUG1SET; ADMUX = ADRIG2;  break;
      case 11: channel = 12; if ( adc_result >= ug1set ) CLKUG1RST; ADMUX = ADRUG1; break;
      case 12: channel = 13; CLKUG1SET; ADMUX = ADRREZ;  break;
      case 13: channel =  0; if ( adc_result >= ug1set ) CLKUG1RST; ADMUX = ADRUG1; break;
   }

   event_push(EVENT_ADC);
}


//*************************************************************
//      Program glowny
//*************************************************************

int main(void)
{
   configure_ports();
   configure_processor();  
   configure_timer();
   configure_pwm();
   configure_adc();
   configure_uart();
   initilize_watchdog();
   initilize_interrupts();

//***** Inicjalizacja zmiennych *******************************

   vref = 509;                                  // vref >= 480
   TOPPWM = 61 * vref / 100;             // okres PWM Ua i Ug2

   ug1set = ug1zer = ug1ref = liczug1( 240 );    // -24.0V
   lamptem.ug1def = 240;
   anode = 28;
   irx = 0;
   tout = MS250; // zeruj odliczanie czasu od ostatniego rx
   lamprem = lamprom[1]; // wstepnie z flasha
   memcpy_P(&lamptem, &lamprom[typ], sizeof(lamptem));
   wartmin = 0;
   wartmax = (ELAMP+FLAMP-1);  // wszystkie lampy
   wart = &typ;                               // wskaz na Typ
   EEPROM_READ((int)&poptyp, typ); // ustaw na ostatnio aktywny
   lastyp = typ;
   ADMUX = ADRIH;

   //***** Inicjalizacja wyswietlacza LCD ************************
   lcd_init();
   send_cyrillic_chars();
   show_splash();

   SEI;                                    // wlacz przerwania
   /* Hold splash ~2s; WDR every 100ms so watchdog doesn't reset the MCU */
   for( i = 0; i < 20; i++ ) {
      WDR;
      if( i == 19 ) { SPKON; }
      _delay_ms( 100 );
      SPKOFF;
   }

// 01234567890123456789
// ====================
// 001 ECC83_J29 G-24.0
// H=12.6V A=300 G2=300
// T 2550mA 199.9 39.99
// S=99.9 u=99.9 R=9999
// ====================

   cstr2rs( "\r\nPress <ESC> to get LCD copy\r\nNr Type Uh[V] Ih[mA] -Ug[V] Ua[V] Ia[mA] Ug2[V] Ig2[mA] S[mA/V] R[k] K[V/V]" );

//***** Glowna petla programu *********************************

   while( 1 )
   {
      /* Drain event ring: run handlers for any events pushed by ISRs */
      while ( event_tail != event_head )
      {
         uint8_t ev = event_ring[event_tail];
         event_tail = (event_tail + 1) % EVENT_RING_SIZE;
         if ( ev & EVENT_TIMER2 ) timer2_tick_handler();
         if ( ev & EVENT_INT1     ) int1_handler();
         if ( ev & EVENT_UART_TXC ) uart_txc_handler();
         if ( ev & EVENT_UART_RXC ) uart_rxc_handler();
         if ( ev & EVENT_ADC      ) adc_handler();
      }
      WDR;
      if ( sync == 1 )
      {
         sync = 0;
         update_display();

//***** Wyswietlenie zawartosci bufora ************************

//          gotoxy( 0, 0 );                              // Numer
//          str2lcd( ((adr == 0) && (start != (FUH+2))), &buf[0] );

//          gotoxy( 3, 0 );                              // Nazwa
//          char2lcd( 0, ' ' );

//          for( i = 0; i < 9; i++ ) char2lcd( (((adr-1) == i) || ((i == 7) && (start == (FUH+2)) && ((buf[11] == '1') || (buf[11] == '2')) )), buf[i+4] );
//          gotoxy( 13, 0 );
//          cstr2lcd_P( 0, PSTR(" G") );
//          char2lcd( (adr == 10), '-' );
//          str2lcd( (adr == 10), &buf[24] );                // Ug1

//          gotoxy( 0, 1 );
//          cstr2lcd_P( 0, PSTR("H=") );
//          str2lcd( (adr == 11), &buf[14] );                 // Uh
//          cstr2lcd_P( 0, PSTR("V A=") );
//          str2lcd( (adr == 13), &buf[29] );                 // Ua
//          cstr2lcd_P( 0, PSTR(" G2=") );
//          str2lcd( (adr == 15), &buf[39] );                // Ug2

//          gotoxy( 0, 2 );
//          errcode = ' ';
//          if( (err & OVERIH) == OVERIH ) errcode = 'H';
//          if( (err & OVERIA) == OVERIA ) errcode = 'A';
//          if( (err & OVERIG) == OVERIG ) errcode = 'G';
//          if( (err & OVERTE) == OVERTE ) errcode = 'T';
//          char2lcd( 0, errcode );                           // Err
//          char2lcd( 0, ' ' );
//          str2lcd( (adr == 12), &buf[19] );                 // Ih
//          cstr2lcd_P( 0, PSTR("mA ") );
//          str2lcd( (adr == 14), &buf[33] );                 // Ia
//          char2lcd( 0, ' ' );
//          str2lcd( (adr == 16), &buf[43] );                // Ig2

//          gotoxy( 0, 3 );
//          if( typ > 1 )
//          {
//             gotoxy( 0, 3 );
//             cstr2lcd_P( 0, PSTR("S=") );
//             str2lcd( (adr == 17), &buf[49] );                 // S

//             gotoxy( 6, 3 );
//             cstr2lcd_P( 0, PSTR(" R=") );
//             str2lcd( (adr == 18), &buf[54] );                 // R

//             gotoxy( 13, 3 );
//             if( (start <= (FUH+2)) || (dusk0 == DMAX) )
//             {
//                cstr2lcd_P( 0, PSTR(" K=") );
//                str2lcd( (adr == 19), &buf[59] );                 // K
//             }
//             else
//             {
//                cstr2lcd_P( 0, PSTR(" T=") );
//                int2asc( start >> 2, ascii);

//                if( ascii[2] != '0' )
//                {
//                   char2lcd( 0, ascii[2] );
//                   char2lcd( 0, ascii[1] );
//                }
//                else
//                {
//                   char2lcd( 0, ' ' );
//                   if( ascii[1] != '0' )
//                   {
//                      char2lcd( 0, ascii[1] );
//                   }
//                   else
//                   {
//                      char2lcd( 0, ' ' );
//                   }
//                }
//                char2lcd( 0, ascii[0] );
//                char2lcd( 0, 's' );
//             }
//          }
//          else
//          {
//             if( typ == 0 ) { cstr2lcd_P( 0, PSTR("                    ") ); } else { cstr2lcd_P( 0, PSTR(" TX/RX: 9600,8,N,1  ") ); }
//          }
//       // }
//       GICR = BIT(INT1);                             // wlacz INT1
//       if( (err != 0) && ((start == 0) || (start > (TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2))) ) { stop = 1; start = (TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2); } // awaryjne wylaczenie

// //***** Wyswietlanie Numeru ***********************************
//       if( adr == 0 )                      // ustawianie numeru
//       {
//          int2asc( typ , ascii);
//          buf[0] = ascii[2];
//          buf[1] = ascii[1];
//          buf[2] = ascii[0];

// //***** Pobieranie nowej Nazwy ************************************
//          if( typ < FLAMP )
//          {
//             if( typ == 1 )
//             {
//                lamptem = lamprem;
//             }
//             else
//             {
//                memcpy_P(&lamptem, &lamprom[typ], sizeof(lamptem));
               
//                if( (typ != lastyp) && (start != (FUH+2)) ) // zmiana typu, poza restartem
//                {
//                  lamprem = lamprom[1]; // kazdy inny kasuje dane remote
//                  anode = 28;
//                  ug1set = ug1zer = ug1ref = liczug1( 240 );    // -24.0V
//                  uhset = ihset = uaset = ug2set = 0;
//                }
//             }
//             tuh = (lamptem.nazwa[8] - '0') * 240;    // 240 = 1min
//             for( i = 0; i < 9; i++ ) buf[i+4] = (uint8_t)lamptem.nazwa[i];
//          }
//          else
//          {
//             EEPROM_READ_BLOCK((int)&lampeep[typ-FLAMP], &lamptem, sizeof(katalog));
//             tuh = (lamptem.nazwa[8] - 27) * 240;     // 240 = 1min
//             for( i = 0; i < 9; i++ ) buf[i+4] = AZ[az_index((uint8_t)lamptem.nazwa[i])];
//          }
//          lastyp = typ;                           //zapamietaj ostatnio pobrany typ
//          nowa = 1;                     // zaktualizowano nazwe
//       }

//       if( (typ == 0) || (typ == 1) )
//       {
//          anode = lamptem.nazwa[7];
//          buf[11] = AZ[az_index((uint8_t)anode)];
//       }

// //***** Edycja Nazwy ******************************************
//       if( typ >= FLAMP )
//       {
//          if( (adr > 0) && (adr < 10) )            // edycja nazwy
//          {
// //***** Wyswietlanie edytowanej Nazwy *************************
//             if( czytaj == 1 ) EEPROM_READ_BLOCK((int)&lampeep[typ-FLAMP].nazwa, lamptem.nazwa, sizeof(lamptem.nazwa));
//             for( i = 0; i < 9; i++ ) buf[i+4] = AZ[az_index((uint8_t)lamptem.nazwa[i])];
//             czytaj = 0;
//             if( dusk0 == DMAX ) zapisz = 1;
//             if( (nodus == DMIN) && (zapisz == 1) )
//             {
//                EEPROM_WRITE_BYTE((int)&lampeep[typ-FLAMP].nazwa[adr-1], lamptem.nazwa[adr-1]);
//                zapisz = 0;
//             }
//          }
//          else
//          {
//             czytaj = 1;
//          }
//       }
// //***** Ustawianie Ug1 ****************************************

//       ug1ref = liczug1( lamptem.ug1def );               // przelicz Ug1

//       temp = mug1adc;
//       temp *= 725;
//       licz = 40960000;
//       if( licz > temp ) { licz -= temp; } else licz = 0;
//       licz >>= 16;     //   /= 65536;
//       licz *= vref;
//       licz /= 1000;
//       ug1 = (uint16_t)licz;
//       if( start == (FUH+2) ) licz = ug1lcd;
//       if( (adr == 0) || (adr == 10) )
//       {
//          if( dusk0 == DMAX )
//          {
//             licz = lamptem.ug1def;              // 0..250..300
//             zapisz = 1;
//          }
//          if( (nodus == DMIN) && (adr == 10) && (zapisz == 1) )
//          {
//             zapisz = 0;
//             if( typ == 0 )                           // SUPPLY
//             {
//                ug1set = ug1ref;
//             }
//             if( typ >= FLAMP )         // ELAMP
//             {
//                EEPROM_WRITE((int)&lampeep[typ-FLAMP].ug1def, lamptem.ug1def);
//             }
//          }
//       }
// //***** Wyswietlanie Ug1 **************************************
//       int2asc( (uint16_t)licz , ascii);
//       buf[24] = (ascii[2] != '0')? ascii[2]: ' ';
//       buf[25] = ascii[1];
//       buf[26] = '.';
//       buf[27] = ascii[0];
// //***** Ustawianie Uh *****************************************
//       licz = muhadc;
//       licz *= vref;
//       licz >>= 14;   //  /= 16384;                      // 0..200
//       temp = mihadc;   // poprawka na spadek napiecia na boczniku
//       temp *= vref;
//       temp >>= 16;         //    /= 65536;
//       if( licz > temp ) { licz -= temp; } else licz = 0;
//       licz /= 10;
//       uh = (uint16_t)licz;
//       if( start == (FUH+2) ) licz = uhlcd;
//       if( (adr == 0) || (adr == 11) )
//       {
//          if( dusk0 == DMAX )
//          {
//             licz = lamptem.uhdef;
//             zapisz = 1;
//          }
//          if( (nodus == DMIN) && (adr == 11) && (zapisz == 1) )
//          {
//             zapisz = 0;
//             if( typ == 0 )                           // SUPPLY
//             {
//                uhset = lamptem.uhdef;
//                ihset = lamptem.ihdef = 0;
//             }
//             if( typ >= FLAMP )                        // ELAMP
//             {
//                EEPROM_WRITE((int)&lampeep[typ-FLAMP].uhdef, lamptem.uhdef);
//             }
//          }
//       }
// //***** Wyswietlanie Uh ***************************************
//       int2asc( (uint16_t)licz, ascii);
//       buf[14] = (ascii[2] != '0')? ascii[2]: ' ';
//       buf[15] = ascii[1];
//       buf[16] = '.';
//       buf[17] = ascii[0];
// //***** Ustawianie Ih *****************************************
//       licz = mihadc;
//       licz *= vref;
//       licz >>= 15;    //   /= 32768;                  // 0..250
//       ih = (uint16_t)licz;
//       if( start == (FUH+2) ) licz = ihlcd;
//       if( (adr == 0) || (adr == 12) )
//       {
//          if( dusk0 == DMAX )
//          {
//             licz = lamptem.ihdef;
//             zapisz = 1;
//          }
//          if( (nodus == DMIN) && (adr == 12) && (zapisz == 1) )
//          {
//             zapisz = 0;
//             if( typ == 0 )                           // SUPPLY
//             {
//                ihset = lamptem.ihdef;
//                uhset = lamptem.uhdef = 0;
//             }
//             if( typ >= FLAMP )     // ELAMP
//             {
//                EEPROM_WRITE((int)&lampeep[typ-FLAMP].ihdef, lamptem.ihdef);
//             }
//          }
//       }
// //***** Wyswietlanie Ih ***************************************
//       int2asc( (uint16_t)licz, ascii);
//       if( ascii[2] != '0' )
//       {
//          buf[19] = ascii[2];
//          buf[20] = ascii[1];
//          buf[21] = ascii[0];
//       }
//       else
//       {
//          buf[19] = ' ';
//          if( ascii[1] != '0' )
//          {
//             buf[20] = ascii[1];
//             buf[21] = ascii[0];
//          }
//          else
//          {
//             buf[20] = ' ';
//             buf[21] = (ascii[0] != '0')? ascii[0]: ' ';
//          }
//       }
//       buf[22] = '0';
// //***** Ustawianie Ua *****************************************
//       licz = muaadc;
//       licz *= vref;
//       licz /= 107436;
//       ua = (uint16_t)licz;
//       if( start == (FUH+2) ) licz = ualcd;
//       if( (adr == 0) || (adr == 13) )
//       {
//          if( dusk0 == DMAX )
//          {
//             licz = lamptem.uadef;
//             zapisz = 1;
//          }
//          if( (nodus == DMIN) && (adr == 13) && (zapisz == 1) )
//          {
//             zapisz = 0;
//             if( typ == 0 )                           // SUPPLY
//             {
//                uaset = lamptem.uadef;
//             }
//             if( typ >= FLAMP )     // ELAMP
//             {
//                EEPROM_WRITE((int)&lampeep[typ-FLAMP].uadef, lamptem.uadef);
//             }
//          }
//       }
// //***** Wyswietlanie Ua ***************************************
//       int2asc( (uint16_t)licz, ascii );
//       if( ascii[2] != '0' )
//       {
//          buf[29] = ascii[2];
//          buf[30] = ascii[1];
//       }
//       else
//       {
//          buf[29] = ' ';
//          buf[30] = (ascii[1] != '0')? ascii[1]: ' ';
//       }
//       buf[31] = ascii[0];
// //***** Ustawianie Ia ***************************************
//       licz = miaadc;
//       licz *= vref;
//       licz >>= 14;       //    /= 16384;
//       temp = muaadc;
//       temp *= vref;
//       if( range == 0 ) { temp *= 10; }
//       temp /= 4369064;
//       if( licz > temp ) { licz -= temp; } else licz = 0;
//       ia = (uint16_t)licz;
//       if( start == (FUH+2) ) licz = ialcd;
//       rangedef = 0;
//       if( (adr == 0) || (adr == 14) )
//       {
//          if( dusk0 == DMAX )
//          {
//             licz = lamptem.iadef;
//             rangedef = 1;
//             zapisz = 1;
//          }
//          if( (nodus == DMIN) && (adr == 14) && (zapisz == 1) )
//          {
//             zapisz = 0;
//             if( typ >= FLAMP )  // ELAMP
//             {
//                EEPROM_WRITE((int)&lampeep[typ-FLAMP].iadef, lamptem.iadef);
//             }
//          }
//       }
// //***** Wyswietlanie Ia ***************************************
//       int2asc( (uint16_t)licz, ascii );
//       if( (rangedef == 0) && (((start != (FUH+2)) && (range == 0)) || ((start == (FUH+2)) && (rangelcd == 0))) )
//       {
//          buf[33] = (ascii[3] != '0')? ascii[3]: ' ';
//          buf[34] = ascii[2];
//          buf[35] = '.';
//          buf[36] = ascii[1];
//          buf[37] = ascii[0];
//       }
//       else
//       {
//          if( ascii[3] != '0' )
//          {
//             buf[33] = ascii[3];
//             buf[34] = ascii[2];
//          }
//          else
//          {
//             buf[33] = ' ';
//             buf[34] = (ascii[2] != '0')? ascii[2]: ' ';
//          }
//          buf[35] = ascii[1];
//          buf[36] = '.';
//          buf[37] = ascii[0];
//       }
// //***** Ustawianie Ug2 ****************************************
//       licz = mug2adc;
//       licz *= vref;
//       licz /= 107436;
//       ug2 = (uint16_t)licz;
//       if( start == (FUH+2) ) licz = ug2lcd;
//       if( (adr == 0) || (adr == 15) )
//       {
//          if( dusk0 == DMAX )
//          {
//             licz = lamptem.ug2def;
//             zapisz = 1;
//          }
//          if( (nodus == DMIN) && (adr == 15) && (zapisz == 1) )
//          {
//             zapisz = 0;
//             if( typ == 0 )                           // SUPPLY
//             {
//                ug2set = lamptem.ug2def;
//             }
//             if( typ >= FLAMP )  // ELAMP
//             {
//                EEPROM_WRITE((int)&lampeep[typ-FLAMP].ug2def, lamptem.ug2def);
//             }
//          }
//       }
// //***** Wyswietlanie Ug2 **************************************
//       int2asc( (uint16_t)licz, ascii );
//       if( ascii[2] != '0' )
//       {
//          buf[39] = ascii[2];
//          buf[40] = ascii[1];
//       }
//       else
//       {
//          buf[39] = ' ';
//          buf[40] = (ascii[1] != '0')? ascii[1]: ' ';
//       }
//       buf[41] = ascii[0];
// //***** Ustawianie Ig2 **************************************
//       licz = mig2adc;
//       licz *= vref;
//       licz >>= 13;   //  /8192(40mA)  /16384(20mA);
//       temp = mug2adc;
//       temp *= vref;
//       temp *= 10;
//       temp /= 4369064;
//       if( licz > temp ) { licz -= temp; } else licz = 0;
//       ig2 = (uint16_t)licz;
//       if( start == (FUH+2) ) licz = ig2lcd;
//       if( (adr == 0) || (adr == 16) )
//       {
//          if( dusk0 == DMAX )
//          {
//             licz = lamptem.ig2def;
//             zapisz = 1;
//          }
//          if( (nodus == DMIN) && (adr == 16) && (zapisz == 1) )
//          {
//             zapisz = 0;
//             if( typ >= FLAMP ) // ELAMP
//             {
//                EEPROM_WRITE((int)&lampeep[typ-FLAMP].ig2def, lamptem.ig2def);
//             }
//          }
//       }
// //***** Wyswietlanie Ig2 **************************************
//       int2asc( (uint16_t)licz, ascii);
//       buf[43] = (ascii[3] != '0')? ascii[3]: ' ';
//       buf[44] = ascii[2];
//       buf[45] = '.';
//       buf[46] = ascii[1];
//       buf[47] = ascii[0];
// //***** Ustawianie S ****************************************
//       licz = s;
//       if( start == (FUH+2) ) licz = slcd;
//       if( (adr == 0) || (adr == 17) )
//       {
//          if( dusk0 == DMAX )
//          {
//             licz = lamptem.sdef;
//             zapisz = 1;
//          }
//          if( (nodus == DMIN) && (adr == 17) && (zapisz == 1) )
//          {
//             zapisz = 0;
//             if( typ >= FLAMP )    // ELAMP
//             {
//                EEPROM_WRITE((int)&lampeep[typ-FLAMP].sdef, lamptem.sdef);
//             }
//          }
//       }
// //***** Wyswietlanie S ****************************************
//       int2asc( licz, ascii );
//       buf[49] = (ascii[2] != '0')? ascii[2]: ' ';
//       buf[50] = ascii[1];
//       buf[51] = '.';
//       buf[52] = ascii[0];
// //***** Ustawianie R ****************************************
//       licz = r;
//       if( start == (FUH+2) ) licz = rlcd;
//       if( (adr == 0) || (adr == 18) )
//       {
//          if( dusk0 == DMAX )
//          {
//             licz = lamptem.rdef;
//             zapisz = 1;
//          }
//          if( (nodus == DMIN) && (adr == 18) && (zapisz == 1) )
//          {
//             zapisz = 0;
//             if( typ >= FLAMP )  // ELAMP
//             {
//                EEPROM_WRITE((int)&lampeep[typ-FLAMP].rdef, lamptem.rdef);
//             }
//          }
//       }
// //***** Wyswietlanie R ****************************************
//       int2asc( licz, ascii );
//       buf[54] = (ascii[2] != '0')? ascii[2]: ' ';
//       buf[55] = ascii[1];
//       buf[56] = '.';
//       buf[57] = ascii[0];
// //***** Ustawianie K ****************************************
//       licz = k;
//       if( start == (FUH+2) ) licz = klcd;
//       if( (adr == 0) || (adr == 19) )
//       {
//          if( dusk0 == DMAX )
//          {
//             licz = lamptem.kdef;
//             zapisz = 1;
//          }
//          if( (nodus == DMIN) && (adr == 19) && (zapisz == 1) )
//          {
//             zapisz = 0;
//             if( typ >= FLAMP )   // ELAMP
//             {
//                EEPROM_WRITE((int)&lampeep[typ-FLAMP].kdef, lamptem.kdef);
//             }
//          }
//       }
// //***** Wyswietlanie K ****************************************
//       int2asc( licz, ascii );

//       if( ascii[3] != '0' )
//       {
//          buf[59] = ascii[3];
//          buf[60] = ascii[2];
//          buf[61] = ascii[1];
//       }
//       else
//       {
//          buf[59] = ' ';
//          if( ascii[2] != '0' )
//          {
//             buf[60] = ascii[2];
//             buf[61] = ascii[1];
//          }
//          else
//          {
//             buf[60] = ' ';
//             buf[61] = (ascii[1] != '0')? ascii[1]: ' ';
//          }
//       }
//       buf[62] = ascii[0];
// //***** Wyslanie pomiarow do PC *******************************
//       if( txen )
//       {
//          EEPROM_WRITE((int)&poptyp, typ);
//          if( typ == 1 )
//          {
//             for( i = 0; i < 10; i++ )
//             {
//                char2rs( bufin[i] );
//             }
//          }
//          else
//          {
//             cstr2rs( "\r\n" );
//             for( i = 0; i < 63; i++ )
//             {
//                if( buf[i] != '\0' ) char2rs( buf[i] ); else cstr2rs( "  " );
//             }
//          }
//          txen = 0;
//       }
   }
}}