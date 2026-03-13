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

// MINIMAL globals here
uint8_t
   ascii[5],
   bufin[10],
   buf[64];

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

void char2rs(uint8_t bajt)
{
   while( !(UCSRA & (1 << UDRE)) ) ;   /* wait for transmit buffer empty (poll; works without interrupts) */
   UDR = bajt;
}

void cstr2rs( const char* q )
{
   while( *q )                             // do konca stringu
   {
      while( !(UCSRA & (1 << UDRE)) ) ;   /* wait for transmit buffer empty (poll; works without interrupts) */
      UDR = *q;
      q++;
   }
}


//*************************************************************************
//                 O B S L U G A   P R Z E R W A N
//*************************************************************************

/* Event ring: ISRs push event bits, main drains and dispatches */
#define EVENT_TIMER2      0x01
#define EVENT_INT1        0x02
#define EVENT_UART_TXC    0x04
#define EVENT_UART_RXC    0x08
#define EVENT_RING_SIZE   32
#define UART_RX_RING_SIZE 8
static volatile uint8_t  event_ring[EVENT_RING_SIZE];
static volatile uint8_t  event_head = 0, event_tail = 0;

/* UART RX byte ring: ISR pushes UDR here, handler pops (one byte per EVENT_UART_RXC) */
static volatile uint8_t  uart_rx_ring[UART_RX_RING_SIZE];
static volatile uint8_t  uart_rx_head = 0, uart_rx_tail = 0;

static inline void event_push(uint8_t ev)
{
   uint8_t next = (event_head + 1) % EVENT_RING_SIZE;
   if (next != event_tail)
   {
      event_ring[event_head] = ev;
      event_head = next;
   }
}

static void int1_handler(void)
{
   if( start > (TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2) )
   {
      if( nodus == DMIN )
      {
         start = (TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2);  // wylaczanie kreciolkiem
         stop = 0;
      }
   }
   else
   {
      if( start == (FUH+2) )
      {
         if( dusk0 == DMAX )
         {
            if( RIGHT )
            {
               if( (nowa == 1) && ((lamptem.nazwa[7] == '1') || (lamptem.nazwa[7] == 28)) ) { typ++; nowa = 0; }
            }
            else
            {
               if( (nowa == 1) && ((lamptem.nazwa[7] == '2') || (lamptem.nazwa[7] == 29)) ) { typ--; nowa = 0; }
            }
            zersrk();
         }
         if( nodus == DMIN )
         {
            start = (FUH+1);
         }
      }
      else
      {
         if( start == 0 )
         {
            if( adr == 0 )                          // edycja numeru
            {
               wartmin = 0;
               wartmax = (FLAMP+ELAMP-1); // PWRSUP+REMOTE+FLASH+EEPROM
               wart = &typ;
            }
            if( (adr > 0) && (adr < 7) )                 // edycja nazwy
            {
               cwartmin = 0;
               cwartmax = 36;
               cwart = &lamptem.nazwa[adr-1];
            }
            if( adr == 7 )              // zmiana nr podstawki zarzenia
            {
               cwartmin = 0;
               cwartmax = 9;
               cwart = &lamptem.nazwa[6];
            }
            if( adr == 8 )                // zmiana nr systemu elektrod
            {
               if( typ == 0 )
               {
                  cwartmin = 28;
               }
               else
               {
                  cwartmin = 27;
               }
               cwartmax = 29;
               cwart = &lamptem.nazwa[7];
            }
            if( adr == 9 )                 // ustawianie czasu zarzenia
            {
               cwartmin = 28;
               cwartmax = 36;
               cwart = &lamptem.nazwa[8];
            }
            if( adr == 10 )                         // ustawianie Ug1
            {
               cwartmin = (typ == 0)? 0: 5;          // 0.5 lub 0
               cwartmax = (typ == 0)? 240: 235;      // -23.5 lub -24.0V
               cwart = &lamptem.ug1def;
            }
            if( adr == 11 )                         // ustawianie Uh
            {
               cwartmin = 0;
               cwartmax = 200;                           // 0..20.0V
               cwart = &lamptem.uhdef;
            }
            if( adr == 12 )                         // ustawianie Ih
            {
               cwartmin = 0;
               cwartmax = 250;                     // 0..2.50A
               cwart = &lamptem.ihdef;
            }
            if( adr == 13 )                         // ustawianie Ua
            {
               wartmin = (typ == 0)? 0: 10;
               wartmax = (typ == 0)? 300: 290;     // 0..300V
               wart = &lamptem.uadef;
            }
            if( adr == 14 )                         // ustawianie Ia
            {
               wartmin = 0;
               wartmax = 2000;                      // 0..200.0mA
               wart = &lamptem.iadef;
            }
            if( adr == 15 )                        // ustawianie Ug2
            {
               wartmin = 0;
               wartmax = 300;                       // 0..300V
               wart = &lamptem.ug2def;
            }
            if( adr == 16 )                         // ustawianie Ig2
            {
               wartmin = 0;
               wartmax = 4000;                      // 0..40.00mA
               wart = &lamptem.ig2def;
            }
            if( adr == 17 )                        // ustawianie S
            {
               wartmin = 0;
               wartmax = 999;                         // 99.9
               wart = &lamptem.sdef;
            }
            if( adr == 18 )                        // ustawianie R
            {
               wartmin = 0;
               wartmax = 999;                      // 99.9
               wart = &lamptem.rdef;
            }
            if( adr == 19 )                        // ustawianie K
            {
               wartmin = 0;
               wartmax = 9999;                       // 9999
               wart = &lamptem.kdef;
            }

            if( RIGHT )
            {
               if( (adr > 0) && (adr < 13) )
               {
                  if( dusk0 == DMAX )
                  {
                     if( (*cwart) < cwartmax ) { (*cwart)++; }
                  }
               }
               else
               {
                  if( dusk0 == DMAX )
                  {
                     if( (*wart) < wartmax ) { (*wart)++; } else { if( adr == 0 ) { (*wart) = 0; } }
                  }
               }
               if( nodus == DMIN )
               {
                  if( typ == 0 ) // PWRSUP
                  {
                     if( adr < 15 ) { adr++; } // nie dojezdzaj do Ig2
                     if( adr == 14 ) { adr = 15; } // przeskocz Ia
                     if( adr == 9 ) { adr = 10; } // przeskocz czas zarzenia
                     if( adr == 1 ) { adr = 8; } // 8 przeskocz nazwe i podstawke
                  }
                  if( typ >= FLAMP ) // katalog zmienny
                  {
                     if( adr < 19 ) { adr++; }
                  }
               }
            }
            else
            {
               if( (adr > 0) && (adr < 13) )
               {
                  if( dusk0 == DMAX )
                  {
                     if( (*cwart) > cwartmin ) { (*cwart)--; }
                  }
               }
               else
               {
                  if( dusk0 == DMAX )
                  {
                     if( (*wart) > wartmin ) { (*wart)--; } else { if( adr == 0 ) { (*wart) = (FLAMP+ELAMP-1); } }
                  }
               }
               if( (nodus == DMIN) && (adr > 0) )
               {
                  if( typ >= FLAMP ) { adr--; }
                  if( typ == 0 )
                  {
                     if( adr > 0 ) { adr--; }
                     if( adr == 7 ) { adr = 0; stop = 0; start = (TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2); } // przeskocz nazwe / wylaczenie
                     if( adr == 9 ) { adr = 8; } // przeskocz nazwe i podstawke
                     if( adr == 14 ) { adr = 13; } // pomin Ia
                  }
               }
            }
         }
      }
   }
}

ISR(INT1_vect)
{
   event_push(EVENT_INT1);
}

ISR(ADC_vect)
{
   switch( kanal )
   {
      case 0:
      {
         srezadc += ADC;
         kanal = 1;
         CLKUG1SET;
         ADMUX = ADRIH;
         break;
      }
      case 1:
      {
         kanal = 2;
         if( ADC >= ug1set ) { CLKUG1RST; }
         ADMUX = ADRUG1;
         break;
      }
      case 2:
      {
         adcih = ADC;
         sihadc += adcih;
         kanal = 3;
         CLKUG1SET;
         ADMUX = ADRUH;
//***** Zabezpieczenie nadpradowe Ih **************************
         if( adcih > 400 )
         {
            if( overih > 0 )
            {
               overih--;
            }
            else
            {
               uhset = ihset = 0;
               err |= OVERIH;
            }
         }
         else
         {
            overih = OVERSAMP;
         }
         break;
      }
      case 3:
      {
         kanal = 4;
         if( ADC >= ug1set ) { CLKUG1RST; }
         ADMUX = ADRUG1;
         break;
      }
      case 4:
      {
         suhadc += ADC;
         kanal = 5;
         CLKUG1SET;
         ADMUX = ADRUA;
         break;
      }
      case 5:
      {
         kanal = 6;
         if( ADC >= ug1set ) { CLKUG1RST; }
         ADMUX = ADRUG1;
         break;
      }
      case 6:
      {
         suaadc += ADC;
         kanal = 7;
         CLKUG1SET;
         ADMUX = ADRIA;
         break;
      }
      case 7:
      {
         kanal = 8;
         if( ADC >= ug1set ) { CLKUG1RST; }
         ADMUX = ADRUG1;
         break;
      }
      case 8:
      {
         adcia = ADC;
         siaadc += adcia;
         kanal = 9;
         CLKUG1SET;
         ADMUX = ADRUG2;
//***** Zabezpieczenie nadpradowe Ia ***************************
         if( (range != 0) && (adcia >= 1020) )
         {
            if( overia > 0 )
            {
               overia--;
            }
            else
            {
               uaset = ug2set = 0;
               PWMUA = PWMUG2 = 0;;
               err |= OVERIA;
            }
         }
         else
         {
            overia = OVERSAMP;
         }
//***** Wystawienie Ua ****************************************
         if( PWMUA < uaset ) PWMUA++;
         if( PWMUA > uaset ) PWMUA--;
//***** Ustawianie wysokiego zakresu pomiarowego Ia ***********
         if( (range == 0) && (adcia > 950) )
         {
            range = 1;
            SET200;
         }
//***** Ustawianie niskiego zakresu pomiarowego Ia ************
         if( ((err & OVERIA) == 0) && (range != 0) && (adcia < 85) )
         {
            range = 0;
            RST200;
         }
         break;
      }
      case 9:
      {
         kanal = 10;
         if( ADC >= ug1set ) { CLKUG1RST; }
         ADMUX = ADRUG1;
         break;
      }
      case 10:
      {
         sug2adc += ADC;
         kanal = 11;
         CLKUG1SET;
         ADMUX = ADRIG2;
         break;
      }
      case 11:
      {
         kanal = 12;
         if( ADC >= ug1set ) { CLKUG1RST; }
         ADMUX = ADRUG1;
         break;
      }
      case 12:
      {
         adcig2 = ADC;
         sig2adc += adcig2;
         kanal = 13;
         CLKUG1SET;
         ADMUX = ADRREZ;
//***** Zabezpieczenie nadpradowe Ig2 **************************
         if( adcig2 >= 1020 )
         {
            if( overig2 > 0 )
            {
               overig2--;
            }
            else
            {
               ug2set = 0;
               PWMUG2 = 0;
               err |= OVERIG;
            }
         }
         else
         {
            overig2 = OVERSAMP;
         }
//***** Wystawienie Ug2 ***************************************
         if( PWMUG2 < ug2set ) PWMUG2++;
         if( PWMUG2 > ug2set ) PWMUG2--;
         break;
      }
      case 13:
      {
         sug1adc += ADC;
         kanal = 0;
         if( ADC >= ug1set ) { CLKUG1RST; }
         ADMUX = ADRUG1;
         if( probki == LPROB )
         {
            mrezadc = srezadc;
            mihadc = sihadc;
            muhadc = suhadc;
            muaadc = suaadc;
            miaadc = siaadc;
            mug2adc = sug2adc;
            mig2adc = sig2adc;
            mug1adc = sug1adc;
            srezadc = sihadc = suhadc = suaadc = siaadc = sug2adc = sig2adc = sug1adc = 0;
            probki = 0;
//***** Szybkie wylaczenie Uh *********************************
            if( (uhset == 0) && (ihset == 0) )
               pwm = 0;
            else
            {
               TCCR0 |= BIT(COM01);               // dolacz OC0
//***** Stabilizacja Uh ***************************************
               if( uhset > 0 )
               {
                  lint = muhadc;
                  lint *= vref;
                  lint >>= 14;        //  /= 16384;                // 0..200
                  tint = mihadc;   // poprawka na spadek napiecia na boczniku
                  tint *= vref;
                  tint >>= 16;        //  /= 65536;
                  if( lint > tint ) { lint -= tint; } else { lint = 0; }
                  lint /= 10;
                  if( (uhset > (uint16_t)lint) && (pwm < 255) ) { pwm++; }
                  if( (uhset < (uint16_t)lint) && (pwm >   0) ) { pwm--; }
               }
//***** Stabilizacja Ih ***************************************
               if( ihset > 0 )
               {
                  lint = mihadc;
                  lint *= vref;
                  lint >>= 15;    //   /= 32768;
                  if( (ihset > (uint16_t)lint) )
                  {
                     if( (pwm < 8) || ((mihadc > 32) && (pwm < 255)) ) { pwm++; } // Uh<0.5V lub Ih>5mA
                  }
                  if( (ihset < (uint16_t)lint) && (pwm >   0) ) { pwm--; }
               }
            }
            if( pwm == 0 )
            {
               TCCR0 &= ~BIT(COM01);                        // odlacz OC0
            }
            else
            {
               TCCR0 |= BIT(COM01);                          // dolacz OC0
            }
            PWMUH = pwm;
//***** Ustawianie/kasowanie znacznika przegrzania ************
            if( mrezadc > HITEMP ) err |= OVERTE;
            if( mrezadc < LOTEMP ) err &= ~OVERTE;
         }
         else
         {
            probki++;
         }
         break;
      }
   }
// NOP;
}

static void uart_txc_handler(void)
{
   busy = 0;
}

static void uart_rxc_handler(void)
{
   uint8_t b;
   if ( uart_rx_tail == uart_rx_head ) return;   /* no byte queued */
   b = uart_rx_ring[uart_rx_tail];
   uart_rx_tail = (uart_rx_tail + 1) % UART_RX_RING_SIZE;

   if( typ == 1 )
   {
      bufin[irx] = b;
      if( irx < 9 )
      {
         irx++;
      }
      else
      {
         bufinta = bufin[6]; bufinta <<= 8; bufinta += bufin[5];
         bufintg2 = bufin[8]; bufintg2 <<= 8; bufintg2 += bufin[7];
         crc = 0;
        //  for( irx = 0; irx < 8; irx++ ) crc = CRC8TABLE[ crc ^ bufin[irx]];

         bufin[9] = crc; // !!!

         if( (((uint16_t)(bufin[6])<<8) + bufin[5]) > 300 ) NOP;

         if( !((bufin[0] != ESC)||
              ((bufin[1] != 28)&&(bufin[1] != 29))||
               (bufin[2] > 240)||
               (bufin[3] > 200)||
               (bufin[4] > 250)||
              ((bufin[3] !=0 ) && (bufin[4] !=0 ))||
               (bufinta > 300)||
               (bufintg2 > 300 )||
               (bufin[9]!=crc)) ) // crc)) ) !!!
         {
            lamprem.nazwa[7] = bufin[1];
            ug1set = liczug1( lamprem.ug1def = bufin[2] );
            uhset = lamprem.uhdef = bufin[3];
            ihset = lamprem.ihdef = bufin[4];
            uaset = lamprem.uadef = bufinta;
            ug2set = lamprem.ug2def = bufintg2;
         }
         irx = 0;
         txen = 1;
      }
      tout = MS250; // zeruj odliczanie czasu od ostatniego
   }
   else
   {
      if( b == ESC ) { txen = 1; }
   }
}

ISR(USART_TXC_vect)
{
   event_push(EVENT_UART_TXC);
}

ISR(USART_RXC_vect)
{
   uint8_t next = (uart_rx_head + 1) % UART_RX_RING_SIZE;
   if ( next != uart_rx_tail )
   {
      uart_rx_ring[uart_rx_head] = UDR;
      uart_rx_head = next;
      event_push(EVENT_UART_RXC);
   }
}

static void timer2_tick_handler(void)
{
   if( zwloka != 0 ) { zwloka--; }
   if( tout > 1 ) { tout--; }
   if( tout == 1 ) { tout = 0; irx = 0; crc = 0; }
   if( anode == 29 ) SETSEL; else RSTSEL;

   if( DUSK0 )
   {
      if( dusk0 < DMAX ) { dusk0++; } else { nodus = 0; }
   }
   else
   {
      if( nodus < DMIN )
      {
         nodus++;
      }
      else
      {
         if( (dusk0 > DMIN) && (dusk0 < DMAX) )
         {
            if( (typ > 1) && (err == 0) )
            {
               if( (start == 0) && (adr == 0) )   // start
               {
                  start = (tuh+(TUG+TUA+TUG2+TMAR+TUG+TMAR+TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2));
                  stop = 1;                 // stop po zakonczeniu pomiaru
                  zersrk();
               }
               if( start == (FUH+2) ) // restart
               {
                  start = (TUG+TUA+TUG2+TMAR+TUG+TMAR+TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2); // ponowny pomiar
                  stop = 1;                    // stop po zakonczeniu pomiaru
                  zersrk();
               }
            }
         }
         dusk0 = 0;
      }
   }
   dziel++;
   if( dziel >= MRUG )
   {
      dziel = 0;
      sync = 1;
      if( start == (tuh+(TUG+TUA+TUG2+TMAR+TUG+TMAR+TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) )
      {
         if( lamptem.uhdef != 0)
         {
            uhset = lamptem.uhdef;
            ihset = lamptem.ihdef = 0;
         }
         if( lamptem.ihdef != 0)
         {
            ihset = lamptem.ihdef;
            uhset = lamptem.uhdef = 0;
         }
      }
      if( start == (    (TUG+TUA+TUG2+TMAR+TUG+TMAR+TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) )
      {
         if( (lamptem.nazwa[7] == '2') || (lamptem.nazwa[7] == 29) ) { anode = 29; } // anoda 2
         ug1set = ug1ref - 11; // UgR
      }
      if( start == (    (    TUA+TUG2+TMAR+TUG+TMAR+TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) )
      {
         uaset = lamptem.uadef;
      }
      if( start == (    (        TUG2+TMAR+TUG+TMAR+TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { ug2set = lamptem.ug2def; }

// wystaw Uh/Ih   UgR   Ua   Ug2            UgL          Ug   UaL           UaR          Ua              Ug2    Ua   Ug   Uh/Ih   SPKON     [SPKOFF]     SPKON      SPKOFF STOP Tx
// czekaj      tuh   TUG  TUA   TUG2    TMAR   TUG   TMAR  TUG   TUA    TMAR   TUA   TMAR  TUA       TMAR   FUG2  FUA  FUG     FUH     BEEP-0      BEEP-1     BEEP-2      2    1  0
// czytaj                           IagR          IagL              IaaL          IaaR        Ug2/Ig2
// oblicz                                          S                               R      K

      if( start == (    (             TMAR+TUG+TMAR+TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { ugr = ug1; iar = (range==0)?ia:ia*10; } // IagR
      if( start == (    (                  TUG+TMAR+TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { ug1set = ug1ref + 11; } // UgL
      if( start == (    (                      TMAR+TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { ugl = ug1; ial = (range==0)?ia:ia*10; if( iar != ial ) { ial -= iar; ugr -= ugl; s = ial; s /= ugr; } else s = 999; } // IagL S
      if( start == (    (                           TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { ug1set = ug1ref; } // Ug
      if( start == (    (                               TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { uaset = lamptem.uadef - 10; } // UaL
      if( start == (    (                                   TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { ual = ua; ial = (range==0)?ia:ia*10; } // IaaL
      if( start == (    (                                        TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { uaset = lamptem.uadef + 10; } // UaR
      if( start == (    (                                            TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { uar = ua; iar = (range==0)?ia:ia*10; if( iar != ial ) { uar -= ual; uar *= 1000; iar -= ial; r = uar; r /= iar; } else r = 999; } // IaaR R
      if( start == (    (                                                 TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { uaset = lamptem.uadef; lint = s; lint *= r; lint += 50; lint /= 100; if( lint < 9999 ) { k = (uint16_t)lint; } else k = 9999; } // K=R*S
      if( start == (    (                                                     TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { uhlcd = uh; ihlcd = ih; ug1lcd = ug1; ualcd = ua; ialcd = ia; rangelcd = range; ug2lcd = ug2; ig2lcd = ig2; slcd = s; rlcd = r; klcd = k; txen = 1; }
      if( start == (    (                                                          FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { ug2set = 0; if( typ == 0 ) lamptem.ug2def = 0; }
      if( start == (    (                                                               FUA+FUG+(BIP-0)+FUH+2)) ) { uaset = 0; if( typ == 0 ) lamptem.uadef = 0; }
      if( start == (    (                                                                   FUG+(BIP-0)+FUH+2)) ) { ug1set = ug1zer; anode = 28; }
      if( start == (    (                                                                       (BIP-0)+FUH+2)) ) { if( stop != 0 ) SPKON; }
      if( start == (    (                                                                       (BIP-1)+FUH+2)) ) { if( err != 0 ) { SPKOFF; uhset = ihset = 0; if( typ == 0 ) { lamptem.uhdef = lamptem.ihdef = 0; } } } // alarm - wylacz tez zarzenie
      if( start == (    (                                                                       (BIP-2)+FUH+2)) ) { if( stop != 0 ) SPKON; }
      if( start == (    (                                                                       (BIP-3)+FUH+2)) ) { SPKOFF; }
      if( start == (    (                                                                               FUH+1)) ) { uhset = ihset = 0; if( typ == 0 ) { lamptem.uhdef = lamptem.ihdef = 0; } }
      if( start == (    (                                                                                   1)) ) { err = 0; zersrk(); }

      takt++;
      takt &= 0x03;
      if( (takt == 2) && (dusk0 == DMAX) ) takt = 0;

      if( start == (FUH+2) )
      {
         if( stop == 0 ) start = (FUH+1);
      }
      else
      {
         if( start > 0 ) { start--; }
      }
   }
}

ISR(TIMER2_COMP_vect)
{
   event_push(EVENT_TIMER2);
}

//*************************************************************************
//                 O B S L U G A   W Y S W I E T L A C Z A
//*************************************************************************
static void lcd_pulse_enable(void)
{
	PORTC |=  (1 << LCD_E);
	_delay_us(1);
	PORTC &= ~(1 << LCD_E);
	_delay_us(100);
}

static void lcd_write_nibble(uint8_t nibble)
{
	PORTC &= ~((1 << PC4) | (1 << PC5) | (1 << PC6) | (1 << PC7));
	PORTC |= (nibble & 0x0F) << 4;
	lcd_pulse_enable();
}

void cmd2lcd( uint8_t rs, uint8_t bajt )
{
   if (rs) PORTC |=  (1 << LCD_RS);
	else    PORTC &= ~(1 << LCD_RS);
   lcd_write_nibble(bajt >> 4);
	lcd_write_nibble(bajt & 0x0F);
}

void gotoxy( uint8_t x, uint8_t y )
{
   // cmd2lcd( 0, 0x80 | (64 * (y % 2) + 20 * (y / 2) + x) ); // 4x20
   static const uint8_t line_addr[4] = { 0x00, 0x40, 0x14, 0x54 };
	if (y > 3 || x > 19) return;
	cmd2lcd(0, 0x80 | (line_addr[y] + x));
}

void char2lcd( uint8_t highlight, uint8_t c )
{
   /* Never send 0x00 (CGRAM 0) or 0xFF (full block) to LCD - can look like "filled squares" from bad/uninit data */
   if (c == 0x00 || c == 0xFF) c = ' ';
   (void)highlight;
   cmd2lcd(1, c);
}

void cstr2lcd( uint8_t highlight, const char* c )
{
   while( *c ) { char2lcd( highlight, (uint8_t)* c++ ); }
}

/* Send string from flash (PROGMEM) to LCD; use for PSTR("...") literals */
void cstr2lcd_P( uint8_t highlight, PGM_P p )
{
   uint8_t ch;
   while( (ch = pgm_read_byte(p)) != 0 )
   {
      if ( ch == 0xFF ) ch = ' ';
      char2lcd( highlight, ch );
      p++;
   }
}

static void lcd_init(void)
{
	/* Wait for LCD power-up (DDRC/PORTC already set in main) */
	_delay_ms(50);

	lcd_write_nibble(0x03);
	_delay_ms(5);
	lcd_write_nibble(0x03);
	_delay_us(150);
	lcd_write_nibble(0x03);
	_delay_us(150);
	lcd_write_nibble(0x02);

	cmd2lcd(0, 0x28);
	cmd2lcd(0, 0x0C);
	cmd2lcd(0, 0x06);
	cmd2lcd(0, 0x01);
	_delay_ms(2);
}

void str2lcd( uint8_t f, uint8_t* c )
{
   uint8_t ch;
   while( (ch = *c) != 0 )
   {
      if ( ch == 0xFF ) ch = ' ';   /* avoid full block on LCD (e.g. bad/external buf) */
      char2lcd( f, ch );
      c++;
   }
}

void int2asc( uint8_t liczba, uint8_t *ascii)
{
   uint8_t i, temp;

   for( i = 0; i < 4; i++ )
   {
      temp = liczba % 10;
      liczba /= 10;
      ascii[i] = '0' + temp;
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

   lcd_init();

   //***** Inicjalizacja wyswietlacza LCD ************************
   cmd2lcd( 0, (0x40 | 0x00) ); for( i = 0; i < 8; i++ ) { cmd2lcd( 1, (uint8_t)cyrB[i] ); }
   cmd2lcd( 0, (0x40 | 0x08) ); for( i = 0; i < 8; i++ ) { cmd2lcd( 1, (uint8_t)cyrC[i] ); }
   cmd2lcd( 0, (0x40 | 0x10) ); for( i = 0; i < 8; i++ ) { cmd2lcd( 1, (uint8_t)cyrD[i] ); }
   cmd2lcd( 0, (0x40 | 0x18) ); for( i = 0; i < 8; i++ ) { cmd2lcd( 1, (uint8_t)cyrF[i] ); }
   cmd2lcd( 0, (0x40 | 0x20) ); for( i = 0; i < 8; i++ ) { cmd2lcd( 1, (uint8_t)cyrG[i] ); }
   cmd2lcd( 0, (0x40 | 0x28) ); for( i = 0; i < 8; i++ ) { cmd2lcd( 1, (uint8_t)cyrI[i] ); }
   cmd2lcd( 0, (0x40 | 0x30) ); for( i = 0; i < 8; i++ ) { cmd2lcd( 1, (uint8_t)cyrP[i] ); }
   cmd2lcd( 0, (0x40 | 0x38) ); for( i = 0; i < 8; i++ ) { cmd2lcd( 1, (uint8_t)cyrZ[i] ); }
   
   // Splash already written above before SEI
   gotoxy( 0, 0 ); cstr2lcd_P( 0, PSTR("   VTTester 2.06    ") );
   gotoxy( 0, 1 ); cstr2lcd_P( 0, PSTR("Tomasz|Adam |       ") );
   gotoxy( 0, 2 ); cstr2lcd_P( 0, PSTR("Gumny |Tatus|       ") );
   gotoxy( 0, 3 ); cstr2lcd_P( 0, PSTR("forum-trioda.pl/    ") );
   SEI;                                    // wlacz przerwania (after init to avoid simavr nested stack)
   for( i = 0; i < 20; i++ ) { WDR; if( i == 19 ) { SPKON; } _delay_ms( 100 ); SPKOFF; };

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
      }
      WDR;
      if ( sync == 1 )
      {
         sync = 0;

//***** Wyswietlenie zawartosci bufora ************************

         gotoxy( 0, 0 );                              // Numer
         str2lcd( ((adr == 0) && (start != (FUH+2))), &buf[0] );

         gotoxy( 3, 0 );                              // Nazwa
         char2lcd( 0, ' ' );

         for( i = 0; i < 9; i++ ) char2lcd( (((adr-1) == i) || ((i == 7) && (start == (FUH+2)) && ((buf[11] == '1') || (buf[11] == '2')) )), buf[i+4] );
         gotoxy( 13, 0 );
         cstr2lcd_P( 0, PSTR(" G") );
         char2lcd( (adr == 10), '-' );
         str2lcd( (adr == 10), &buf[24] );                // Ug1

         gotoxy( 0, 1 );
         cstr2lcd_P( 0, PSTR("H=") );
         str2lcd( (adr == 11), &buf[14] );                 // Uh
         cstr2lcd_P( 0, PSTR("V A=") );
         str2lcd( (adr == 13), &buf[29] );                 // Ua
         cstr2lcd_P( 0, PSTR(" G2=") );
         str2lcd( (adr == 15), &buf[39] );                // Ug2

         gotoxy( 0, 2 );
         errcode = ' ';
         if( (err & OVERIH) == OVERIH ) errcode = 'H';
         if( (err & OVERIA) == OVERIA ) errcode = 'A';
         if( (err & OVERIG) == OVERIG ) errcode = 'G';
         if( (err & OVERTE) == OVERTE ) errcode = 'T';
         char2lcd( 0, errcode );                           // Err
         char2lcd( 0, ' ' );
         str2lcd( (adr == 12), &buf[19] );                 // Ih
         cstr2lcd_P( 0, PSTR("mA ") );
         str2lcd( (adr == 14), &buf[33] );                 // Ia
         char2lcd( 0, ' ' );
         str2lcd( (adr == 16), &buf[43] );                // Ig2

         gotoxy( 0, 3 );
         if( typ > 1 )
         {
            gotoxy( 0, 3 );
            cstr2lcd_P( 0, PSTR("S=") );
            str2lcd( (adr == 17), &buf[49] );                 // S

            gotoxy( 6, 3 );
            cstr2lcd_P( 0, PSTR(" R=") );
            str2lcd( (adr == 18), &buf[54] );                 // R

            gotoxy( 13, 3 );
            if( (start <= (FUH+2)) || (dusk0 == DMAX) )
            {
               cstr2lcd_P( 0, PSTR(" K=") );
               str2lcd( (adr == 19), &buf[59] );                 // K
            }
            else
            {
               cstr2lcd_P( 0, PSTR(" T=") );
               int2asc( start >> 2, ascii);

               if( ascii[2] != '0' )
               {
                  char2lcd( 0, ascii[2] );
                  char2lcd( 0, ascii[1] );
               }
               else
               {
                  char2lcd( 0, ' ' );
                  if( ascii[1] != '0' )
                  {
                     char2lcd( 0, ascii[1] );
                  }
                  else
                  {
                     char2lcd( 0, ' ' );
                  }
               }
               char2lcd( 0, ascii[0] );
               char2lcd( 0, 's' );
            }
         }
         else
         {
            if( typ == 0 ) { cstr2lcd_P( 0, PSTR("                    ") ); } else { cstr2lcd_P( 0, PSTR(" TX/RX: 9600,8,N,1  ") ); }
         }
      // }
      GICR = BIT(INT1);                             // wlacz INT1
      if( (err != 0) && ((start == 0) || (start > (TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2))) ) { stop = 1; start = (TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2); } // awaryjne wylaczenie

//***** Wyswietlanie Numeru ***********************************
      if( adr == 0 )                      // ustawianie numeru
      {
         int2asc( typ , ascii);
         buf[0] = ascii[2];
         buf[1] = ascii[1];
         buf[2] = ascii[0];

//***** Pobieranie nowej Nazwy ************************************
         if( typ < FLAMP )
         {
            if( typ == 1 )
            {
               lamptem = lamprem;
            }
            else
            {
               memcpy_P(&lamptem, &lamprom[typ], sizeof(lamptem));
               
               if( (typ != lastyp) && (start != (FUH+2)) ) // zmiana typu, poza restartem
               {
                 lamprem = lamprom[1]; // kazdy inny kasuje dane remote
                 anode = 28;
                 ug1set = ug1zer = ug1ref = liczug1( 240 );    // -24.0V
                 uhset = ihset = uaset = ug2set = 0;
               }
            }
            tuh = (lamptem.nazwa[8] - '0') * 240;    // 240 = 1min
            for( i = 0; i < 9; i++ ) buf[i+4] = (uint8_t)lamptem.nazwa[i];
         }
         else
         {
            EEPROM_READ_BLOCK((int)&lampeep[typ-FLAMP], &lamptem, sizeof(katalog));
            tuh = (lamptem.nazwa[8] - 27) * 240;     // 240 = 1min
            for( i = 0; i < 9; i++ ) buf[i+4] = AZ[az_index((uint8_t)lamptem.nazwa[i])];
         }
         lastyp = typ;                           //zapamietaj ostatnio pobrany typ
         nowa = 1;                     // zaktualizowano nazwe
      }

      if( (typ == 0) || (typ == 1) )
      {
         anode = lamptem.nazwa[7];
         buf[11] = AZ[az_index((uint8_t)anode)];
      }

//***** Edycja Nazwy ******************************************
      if( typ >= FLAMP )
      {
         if( (adr > 0) && (adr < 10) )            // edycja nazwy
         {
//***** Wyswietlanie edytowanej Nazwy *************************
            if( czytaj == 1 ) EEPROM_READ_BLOCK((int)&lampeep[typ-FLAMP].nazwa, lamptem.nazwa, sizeof(lamptem.nazwa));
            for( i = 0; i < 9; i++ ) buf[i+4] = AZ[az_index((uint8_t)lamptem.nazwa[i])];
            czytaj = 0;
            if( dusk0 == DMAX ) zapisz = 1;
            if( (nodus == DMIN) && (zapisz == 1) )
            {
               EEPROM_WRITE_BYTE((int)&lampeep[typ-FLAMP].nazwa[adr-1], lamptem.nazwa[adr-1]);
               zapisz = 0;
            }
         }
         else
         {
            czytaj = 1;
         }
      }
//***** Ustawianie Ug1 ****************************************

      ug1ref = liczug1( lamptem.ug1def );               // przelicz Ug1

      temp = mug1adc;
      temp *= 725;
      licz = 40960000;
      if( licz > temp ) { licz -= temp; } else licz = 0;
      licz >>= 16;     //   /= 65536;
      licz *= vref;
      licz /= 1000;
      ug1 = (uint16_t)licz;
      if( start == (FUH+2) ) licz = ug1lcd;
      if( (adr == 0) || (adr == 10) )
      {
         if( dusk0 == DMAX )
         {
            licz = lamptem.ug1def;              // 0..250..300
            zapisz = 1;
         }
         if( (nodus == DMIN) && (adr == 10) && (zapisz == 1) )
         {
            zapisz = 0;
            if( typ == 0 )                           // SUPPLY
            {
               ug1set = ug1ref;
            }
            if( typ >= FLAMP )         // ELAMP
            {
               EEPROM_WRITE((int)&lampeep[typ-FLAMP].ug1def, lamptem.ug1def);
            }
         }
      }
//***** Wyswietlanie Ug1 **************************************
      int2asc( (uint16_t)licz , ascii);
      buf[24] = (ascii[2] != '0')? ascii[2]: ' ';
      buf[25] = ascii[1];
      buf[26] = '.';
      buf[27] = ascii[0];
//***** Ustawianie Uh *****************************************
      licz = muhadc;
      licz *= vref;
      licz >>= 14;   //  /= 16384;                      // 0..200
      temp = mihadc;   // poprawka na spadek napiecia na boczniku
      temp *= vref;
      temp >>= 16;         //    /= 65536;
      if( licz > temp ) { licz -= temp; } else licz = 0;
      licz /= 10;
      uh = (uint16_t)licz;
      if( start == (FUH+2) ) licz = uhlcd;
      if( (adr == 0) || (adr == 11) )
      {
         if( dusk0 == DMAX )
         {
            licz = lamptem.uhdef;
            zapisz = 1;
         }
         if( (nodus == DMIN) && (adr == 11) && (zapisz == 1) )
         {
            zapisz = 0;
            if( typ == 0 )                           // SUPPLY
            {
               uhset = lamptem.uhdef;
               ihset = lamptem.ihdef = 0;
            }
            if( typ >= FLAMP )                        // ELAMP
            {
               EEPROM_WRITE((int)&lampeep[typ-FLAMP].uhdef, lamptem.uhdef);
            }
         }
      }
//***** Wyswietlanie Uh ***************************************
      int2asc( (uint16_t)licz, ascii);
      buf[14] = (ascii[2] != '0')? ascii[2]: ' ';
      buf[15] = ascii[1];
      buf[16] = '.';
      buf[17] = ascii[0];
//***** Ustawianie Ih *****************************************
      licz = mihadc;
      licz *= vref;
      licz >>= 15;    //   /= 32768;                  // 0..250
      ih = (uint16_t)licz;
      if( start == (FUH+2) ) licz = ihlcd;
      if( (adr == 0) || (adr == 12) )
      {
         if( dusk0 == DMAX )
         {
            licz = lamptem.ihdef;
            zapisz = 1;
         }
         if( (nodus == DMIN) && (adr == 12) && (zapisz == 1) )
         {
            zapisz = 0;
            if( typ == 0 )                           // SUPPLY
            {
               ihset = lamptem.ihdef;
               uhset = lamptem.uhdef = 0;
            }
            if( typ >= FLAMP )     // ELAMP
            {
               EEPROM_WRITE((int)&lampeep[typ-FLAMP].ihdef, lamptem.ihdef);
            }
         }
      }
//***** Wyswietlanie Ih ***************************************
      int2asc( (uint16_t)licz, ascii);
      if( ascii[2] != '0' )
      {
         buf[19] = ascii[2];
         buf[20] = ascii[1];
         buf[21] = ascii[0];
      }
      else
      {
         buf[19] = ' ';
         if( ascii[1] != '0' )
         {
            buf[20] = ascii[1];
            buf[21] = ascii[0];
         }
         else
         {
            buf[20] = ' ';
            buf[21] = (ascii[0] != '0')? ascii[0]: ' ';
         }
      }
      buf[22] = '0';
//***** Ustawianie Ua *****************************************
      licz = muaadc;
      licz *= vref;
      licz /= 107436;
      ua = (uint16_t)licz;
      if( start == (FUH+2) ) licz = ualcd;
      if( (adr == 0) || (adr == 13) )
      {
         if( dusk0 == DMAX )
         {
            licz = lamptem.uadef;
            zapisz = 1;
         }
         if( (nodus == DMIN) && (adr == 13) && (zapisz == 1) )
         {
            zapisz = 0;
            if( typ == 0 )                           // SUPPLY
            {
               uaset = lamptem.uadef;
            }
            if( typ >= FLAMP )     // ELAMP
            {
               EEPROM_WRITE((int)&lampeep[typ-FLAMP].uadef, lamptem.uadef);
            }
         }
      }
//***** Wyswietlanie Ua ***************************************
      int2asc( (uint16_t)licz, ascii );
      if( ascii[2] != '0' )
      {
         buf[29] = ascii[2];
         buf[30] = ascii[1];
      }
      else
      {
         buf[29] = ' ';
         buf[30] = (ascii[1] != '0')? ascii[1]: ' ';
      }
      buf[31] = ascii[0];
//***** Ustawianie Ia ***************************************
      licz = miaadc;
      licz *= vref;
      licz >>= 14;       //    /= 16384;
      temp = muaadc;
      temp *= vref;
      if( range == 0 ) { temp *= 10; }
      temp /= 4369064;
      if( licz > temp ) { licz -= temp; } else licz = 0;
      ia = (uint16_t)licz;
      if( start == (FUH+2) ) licz = ialcd;
      rangedef = 0;
      if( (adr == 0) || (adr == 14) )
      {
         if( dusk0 == DMAX )
         {
            licz = lamptem.iadef;
            rangedef = 1;
            zapisz = 1;
         }
         if( (nodus == DMIN) && (adr == 14) && (zapisz == 1) )
         {
            zapisz = 0;
            if( typ >= FLAMP )  // ELAMP
            {
               EEPROM_WRITE((int)&lampeep[typ-FLAMP].iadef, lamptem.iadef);
            }
         }
      }
//***** Wyswietlanie Ia ***************************************
      int2asc( (uint16_t)licz, ascii );
      if( (rangedef == 0) && (((start != (FUH+2)) && (range == 0)) || ((start == (FUH+2)) && (rangelcd == 0))) )
      {
         buf[33] = (ascii[3] != '0')? ascii[3]: ' ';
         buf[34] = ascii[2];
         buf[35] = '.';
         buf[36] = ascii[1];
         buf[37] = ascii[0];
      }
      else
      {
         if( ascii[3] != '0' )
         {
            buf[33] = ascii[3];
            buf[34] = ascii[2];
         }
         else
         {
            buf[33] = ' ';
            buf[34] = (ascii[2] != '0')? ascii[2]: ' ';
         }
         buf[35] = ascii[1];
         buf[36] = '.';
         buf[37] = ascii[0];
      }
//***** Ustawianie Ug2 ****************************************
      licz = mug2adc;
      licz *= vref;
      licz /= 107436;
      ug2 = (uint16_t)licz;
      if( start == (FUH+2) ) licz = ug2lcd;
      if( (adr == 0) || (adr == 15) )
      {
         if( dusk0 == DMAX )
         {
            licz = lamptem.ug2def;
            zapisz = 1;
         }
         if( (nodus == DMIN) && (adr == 15) && (zapisz == 1) )
         {
            zapisz = 0;
            if( typ == 0 )                           // SUPPLY
            {
               ug2set = lamptem.ug2def;
            }
            if( typ >= FLAMP )  // ELAMP
            {
               EEPROM_WRITE((int)&lampeep[typ-FLAMP].ug2def, lamptem.ug2def);
            }
         }
      }
//***** Wyswietlanie Ug2 **************************************
      int2asc( (uint16_t)licz, ascii );
      if( ascii[2] != '0' )
      {
         buf[39] = ascii[2];
         buf[40] = ascii[1];
      }
      else
      {
         buf[39] = ' ';
         buf[40] = (ascii[1] != '0')? ascii[1]: ' ';
      }
      buf[41] = ascii[0];
//***** Ustawianie Ig2 **************************************
      licz = mig2adc;
      licz *= vref;
      licz >>= 13;   //  /8192(40mA)  /16384(20mA);
      temp = mug2adc;
      temp *= vref;
      temp *= 10;
      temp /= 4369064;
      if( licz > temp ) { licz -= temp; } else licz = 0;
      ig2 = (uint16_t)licz;
      if( start == (FUH+2) ) licz = ig2lcd;
      if( (adr == 0) || (adr == 16) )
      {
         if( dusk0 == DMAX )
         {
            licz = lamptem.ig2def;
            zapisz = 1;
         }
         if( (nodus == DMIN) && (adr == 16) && (zapisz == 1) )
         {
            zapisz = 0;
            if( typ >= FLAMP ) // ELAMP
            {
               EEPROM_WRITE((int)&lampeep[typ-FLAMP].ig2def, lamptem.ig2def);
            }
         }
      }
//***** Wyswietlanie Ig2 **************************************
      int2asc( (uint16_t)licz, ascii);
      buf[43] = (ascii[3] != '0')? ascii[3]: ' ';
      buf[44] = ascii[2];
      buf[45] = '.';
      buf[46] = ascii[1];
      buf[47] = ascii[0];
//***** Ustawianie S ****************************************
      licz = s;
      if( start == (FUH+2) ) licz = slcd;
      if( (adr == 0) || (adr == 17) )
      {
         if( dusk0 == DMAX )
         {
            licz = lamptem.sdef;
            zapisz = 1;
         }
         if( (nodus == DMIN) && (adr == 17) && (zapisz == 1) )
         {
            zapisz = 0;
            if( typ >= FLAMP )    // ELAMP
            {
               EEPROM_WRITE((int)&lampeep[typ-FLAMP].sdef, lamptem.sdef);
            }
         }
      }
//***** Wyswietlanie S ****************************************
      int2asc( licz, ascii );
      buf[49] = (ascii[2] != '0')? ascii[2]: ' ';
      buf[50] = ascii[1];
      buf[51] = '.';
      buf[52] = ascii[0];
//***** Ustawianie R ****************************************
      licz = r;
      if( start == (FUH+2) ) licz = rlcd;
      if( (adr == 0) || (adr == 18) )
      {
         if( dusk0 == DMAX )
         {
            licz = lamptem.rdef;
            zapisz = 1;
         }
         if( (nodus == DMIN) && (adr == 18) && (zapisz == 1) )
         {
            zapisz = 0;
            if( typ >= FLAMP )  // ELAMP
            {
               EEPROM_WRITE((int)&lampeep[typ-FLAMP].rdef, lamptem.rdef);
            }
         }
      }
//***** Wyswietlanie R ****************************************
      int2asc( licz, ascii );
      buf[54] = (ascii[2] != '0')? ascii[2]: ' ';
      buf[55] = ascii[1];
      buf[56] = '.';
      buf[57] = ascii[0];
//***** Ustawianie K ****************************************
      licz = k;
      if( start == (FUH+2) ) licz = klcd;
      if( (adr == 0) || (adr == 19) )
      {
         if( dusk0 == DMAX )
         {
            licz = lamptem.kdef;
            zapisz = 1;
         }
         if( (nodus == DMIN) && (adr == 19) && (zapisz == 1) )
         {
            zapisz = 0;
            if( typ >= FLAMP )   // ELAMP
            {
               EEPROM_WRITE((int)&lampeep[typ-FLAMP].kdef, lamptem.kdef);
            }
         }
      }
//***** Wyswietlanie K ****************************************
      int2asc( licz, ascii );

      if( ascii[3] != '0' )
      {
         buf[59] = ascii[3];
         buf[60] = ascii[2];
         buf[61] = ascii[1];
      }
      else
      {
         buf[59] = ' ';
         if( ascii[2] != '0' )
         {
            buf[60] = ascii[2];
            buf[61] = ascii[1];
         }
         else
         {
            buf[60] = ' ';
            buf[61] = (ascii[1] != '0')? ascii[1]: ' ';
         }
      }
      buf[62] = ascii[0];
//***** Wyslanie pomiarow do PC *******************************
      if( txen )
      {
         EEPROM_WRITE((int)&poptyp, typ);
         if( typ == 1 )
         {
            for( i = 0; i < 10; i++ )
            {
               char2rs( bufin[i] );
            }
         }
         else
         {
            cstr2rs( "\r\n" );
            for( i = 0; i < 63; i++ )
            {
               if( buf[i] != '\0' ) char2rs( buf[i] ); else cstr2rs( "  " );
            }
         }
         txen = 0;
      }
   }
}}