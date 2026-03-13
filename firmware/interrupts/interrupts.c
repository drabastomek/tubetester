#include "interrupts.h"
#include "config/config.h"
#include "utils/utils.h"
#include <avr/io.h>

volatile uint8_t uart_rx_ring[UART_RX_RING_SIZE];
volatile uint8_t uart_rx_head = 0, uart_rx_tail = 0;

volatile uint16_t adc_result;
volatile uint8_t adc_channel;

void int1_handler(void)
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

void timer2_tick_handler(void)
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

void uart_txc_handler(void)
{
   busy = 0;
}

void uart_rxc_handler(void)
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

/* Process one ADC result. ch = adc_channel is the slot index; the result r is from
   the physical channel we set in the previous ISR (see ISR(ADC_vect) in main).
   Slot → result from:  0=UG1 1=IH 2=UH 3=UG1 4=UA 5=UG1 6=IA 7=UG1 8=UG2 9=UG1 10=IG2 11=UG1 12=REZ 13=UG1 */
void adc_handler(void)
{
   uint16_t r = adc_result;
   uint8_t ch = adc_channel;

   switch ( ch )
   {
      case 0:   // result = UG1 (used for srezadc / temp path)
         srezadc += r;
         break;
      case 1:   // result = IH
         if ( r >= ug1set ) { CLKUG1RST; }
         break;
      case 2:   // result = UH
         adcih = r;
         sihadc += adcih;
         if ( adcih > 400 )
         {
            if ( overih > 0 ) overih--;
            else { uhset = ihset = 0; err |= OVERIH; }
         }
         else
            overih = OVERSAMP;
         break;
      case 3:   // result = UG1
         if ( r >= ug1set ) { CLKUG1RST; }
         break;
      case 4:   // result = UA
         suhadc += r;
         break;
      case 5:   // result = UG1
         if ( r >= ug1set ) { CLKUG1RST; }
         break;
      case 6:   // result = IA
         suaadc += r;
         break;
      case 7:   // result = UG1
         if ( r >= ug1set ) { CLKUG1RST; }
         break;
      case 8:   // result = UG2 (anode voltage)
         adcia = r;
         siaadc += adcia;
         if ( (range != 0) && (adcia >= 1020) )
         {
            if ( overia > 0 ) overia--;
            else { uaset = ug2set = 0; PWMUA = PWMUG2 = 0; err |= OVERIA; }
         }
         else
            overia = OVERSAMP;
         if ( PWMUA < uaset ) PWMUA++;
         if ( PWMUA > uaset ) PWMUA--;
         if ( (range == 0) && (adcia > 950) ) { range = 1; SET200; }
         if ( ((err & OVERIA) == 0) && (range != 0) && (adcia < 85) ) { range = 0; RST200; }
         break;
      case 9:   // result = UG1
         if ( r >= ug1set ) { CLKUG1RST; }
         break;
      case 10:   // result = IG2
         sug2adc += r;
         break;
      case 11:   // result = UG1
         if ( r >= ug1set ) { CLKUG1RST; }
         break;
      case 12:   // result = UG1
         adcig2 = r;
         sig2adc += adcig2;
         if ( adcig2 >= 1020 )
         {
            if ( overig2 > 0 ) overig2--;
            else { ug2set = 0; PWMUG2 = 0; err |= OVERIG; }
         }
         else
            overig2 = OVERSAMP;
         if ( PWMUG2 < ug2set ) PWMUG2++;
         if ( PWMUG2 > ug2set ) PWMUG2--;
         break;
      case 13:   // result = REZ (temp); then LPROB averaging & Uh/Ih PWM
         sug1adc += r;
         if ( r >= ug1set ) { CLKUG1RST; }
         if ( probki == LPROB )
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
            if ( (uhset == 0) && (ihset == 0) )
               pwm = 0;
            else
            {
               TCCR0 |= BIT(COM01);
               if ( uhset > 0 )
               {
                  lint = muhadc;
                  lint *= vref;
                  lint >>= 14;
                  tint = mihadc;
                  tint *= vref;
                  tint >>= 16;
                  if ( lint > tint ) lint -= tint; else lint = 0;
                  lint /= 10;
                  if ( (uhset > (uint16_t)lint) && (pwm < 255) ) pwm++;
                  if ( (uhset < (uint16_t)lint) && (pwm > 0) ) pwm--;
               }
               if ( ihset > 0 )
               {
                  lint = mihadc;
                  lint *= vref;
                  lint >>= 15;
                  if ( (ihset > (uint16_t)lint) )
                  {
                     if ( (pwm < 8) || ((mihadc > 32) && (pwm < 255)) ) pwm++;
                  }
                  if ( (ihset < (uint16_t)lint) && (pwm > 0) ) pwm--;
               }
            }
            if ( pwm == 0 )
               TCCR0 &= ~BIT(COM01);
            else
               TCCR0 |= BIT(COM01);
            PWMUH = pwm;
            if ( mrezadc > HITEMP ) err |= OVERTE;
            if ( mrezadc < LOTEMP ) err &= ~OVERTE;
         }
         else
            probki++;
         break;
   }
}