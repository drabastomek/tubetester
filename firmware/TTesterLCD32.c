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
 * So when we enter with channel=K, the result we have is from slot K-1 (or 13 when K=0):
 *   K=0→UG1 K=1→IH K=2→UG1 K=3→UH K=4→UG1 K=5→UA K=6→UG1 K=7→IA
 *   K=8→UG1 K=9→UG2 K=10→UG1 K=11→IG2 K=12→UG1 K=13→REZ UG1 is sampled every other slot for the ug1set
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
   memcpy_P(&lamprem, &lamprom[1], sizeof(lamprem)); // wstepnie z flasha
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
      }
   }
}