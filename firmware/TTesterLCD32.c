//*************************************************************
//                           TESTER
//*************************************************************
// OCD  JTAG SPI CK  EE   BOOT BOOT BOOT BOD   BOD SU SU CK   CK   CK   CK
// EN    EN  EN  OPT SAVE SZ1  SZ0  RST  LEVEL EN  T1 T0 SEL3 SEL2 SEL1 SEL0
//  1     0   0   1   1    0    0    1    1     1   1  0  0    0    0    1 (0x99E1 DEFAULT)
//  1     1   0   0   1    0    0    1    0     0   0  1  1    1    1    1 (0xC91F)
//*************************************************************

#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include "config/config.h"
#include "protocol/vttester_remote.h"
#include "utils/utils.h"

/* avr-libc EEPROM API compatibility with IAR-style EEPROM_READ/EEPROM_WRITE */
#define EEPROM_READ(addr, var)   eeprom_read_block((void*)&(var), (const void*)(addr), sizeof(var))
#define EEPROM_WRITE(addr, val) eeprom_write_block((const void*)&(val), (void*)(addr), sizeof(val))

#pragma data:data

unsigned char
   d, i,
   busy,
   sync,
   txen,
   *cwart, cwartmin, cwartmax,
   adr, adrmin, adrmax,
   nowa,
   stop,
   zwloka,
   dziel,
   nodus, dusk0,
   zapisz, czytaj,
   range, rangelcd, rangedef,
   ascii[5],
   kanal,
   takt,
   overih, overia, overig2, err, errcode,
   probki,
   pwm,
   anode,
   irx, tout, crc,
   bufin[10],
   buf[64],
   rx_proto_buf[VTTESTER_FRAME_LEN],
   rx_proto_pos,
   rx_proto_ready,
   tx_proto_buf[VTTESTER_FRAME_LEN],
   remote_meas_trigger,
   remote_meas_pending,
   remote_meas_ready,
   remote_meas_index;

unsigned int
   *wart, wartmin, wartmax,
   start, tuh,
   vref,
   adcih, adcia, adcig2,
   uhset,  ihset,  ug1set,  uaset,  iaset,  ug2set,   ig2set,
   suhadc, sihadc, sug1adc, suaadc, siaadc, sug2adc, sig2adc,
   muhadc, mihadc, mug1adc, muaadc, miaadc, mug2adc, mig2adc,
   ug1zer, ug1ref,
   uh, ih,
   ua, ual, uar,
   ia, ial, iar,
   ug2, ig2,
   ug1, ugl, ugr,
   s, r, k,
   typ, lastyp,
   uhlcd, ihlcd, ug1lcd, ualcd, ialcd, ug2lcd, ig2lcd, slcd, rlcd, klcd,
   srezadc, mrezadc,
   bufinta, bufintg2;

unsigned long
   lint, tint, licz, temp;

katalog
   lamprem,
   lamptem;

//*************************************************************************
//                 O B S L U G A   P R Z E R W A N
//*************************************************************************

ISR(INT1_vect)
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
                  if( (uhset > (unsigned int)lint) && (pwm < 255) ) { pwm++; }
                  if( (uhset < (unsigned int)lint) && (pwm >   0) ) { pwm--; }
               }
//***** Stabilizacja Ih ***************************************
               if( ihset > 0 )
               {
                  lint = mihadc;
                  lint *= vref;
                  lint >>= 15;    //   /= 32768;
                  if( (ihset > (unsigned int)lint) )
                  {
                     if( (pwm < 8) || ((mihadc > 32) && (pwm < 255)) ) { pwm++; } // Uh<0.5V lub Ih>5mA
                  }
                  if( (ihset < (unsigned int)lint) && (pwm >   0) ) { pwm--; }
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

ISR(USART_TXC_vect)
{
   busy = 0;
}

ISR(USART_RXC_vect)
{
   /* Feed byte to VTTester 8-byte protocol buffer */
   rx_proto_buf[rx_proto_pos++] = UDR;
   if( rx_proto_pos >= VTTESTER_FRAME_LEN ) { rx_proto_pos = 0; rx_proto_ready = 1; }
   /* 10-byte legacy path: remote (typ==1) only. Power supply (typ==0) uses 8-byte protocol only. */
   if( typ == 1 )
   {
      bufin[irx] = UDR;
      if( irx < 9 )
      {
         irx++;
      }
      else
      {
         bufinta = bufin[6]; bufinta <<= 8; bufinta += bufin[5];
         bufintg2 = bufin[8]; bufintg2 <<= 8; bufintg2 += bufin[7];

         if( (((unsigned int)(bufin[6])<<8) + bufin[5]) > 300 ) NOP;

         /* 10-byte legacy path: no CRC (backward-compatible LCD dump); byte 9 unused */
         if( !((bufin[0] != ESC)||
              ((bufin[1] != 28)&&(bufin[1] != 29))||
               (bufin[2] > 240)||
               (bufin[3] > 200)||
               (bufin[4] > 250)||
              ((bufin[3] !=0 ) && (bufin[4] !=0 ))||
               (bufinta > 300)||
               (bufintg2 > 300 )) )
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
      if( UDR == ESC ) { txen = 1; }
   }
}

ISR(TIMER2_COMP_vect)
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
      if( remote_meas_trigger && (start == 0) && (err == 0) && (typ == 1) )
      {
         start = (tuh+(TUG+TUA+TUG2+TMAR+TUG+TMAR+TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2));
         stop = 1;
         zersrk();
         remote_meas_trigger = 0;
      }
//***** Sekwencja zalaczanie/wylaczanie napiec ****************
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
      if( start == (    (                                                 TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { uaset = lamptem.uadef; lint = s; lint *= r; lint += 50; lint /= 100; if( lint < 9999 ) { k = (unsigned int)lint; } else k = 9999; } // K=R*S
      if( start == (    (                                                     TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { uhlcd = uh; ihlcd = ih; ug1lcd = ug1; ualcd = ua; ialcd = ia; rangelcd = range; ug2lcd = ug2; ig2lcd = ig2; slcd = s; rlcd = r; klcd = k; txen = 1; if( remote_meas_pending ) remote_meas_ready = 1; }
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

//*************************************************************************
//                 O B S L U G A   W Y S W I E T L A C Z A
//*************************************************************************

void cmd2lcd( char rs, char bajt )
{
   delay( 1 );
   if( rs ) { RSSET; } else { RSRST; }
   PORTC &= 0x0f;
   PORTC |= ( bajt & 0xf0 );
   ENSET;
   NOP2; NOP2; NOP2; NOP2;
   ENRST;
   PORTC &= 0x0f;
   PORTC |= ( (bajt << 4) & 0xf0 );
   ENSET;
   NOP2; NOP2; NOP2; NOP2;
   ENRST;
}

void gotoxy( char x, char y )
{
   cmd2lcd( 0, 0x80 | (64 * (y % 2) + 20 * (y / 2) + x) ); // 4x20
}

void char2lcd( char f, char c )
{
   cmd2lcd( 1, ( (f == 1) && (takt == 0) )? ' ': c );
}

void cstr2lcd( char f, const unsigned char* c )
{
   while( *c ) { char2lcd( f, *c ); c++; }
}

void str2lcd( char f, unsigned char* c )
{
   while( *c ) { char2lcd( f, *c ); c++; }
}

void int2asc( unsigned int liczba )
{
   unsigned char
   i, temp;

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

void main(void)
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
   rx_proto_pos = 0;
   rx_proto_ready = 0;
   remote_meas_trigger = 0;
   remote_meas_pending = 0;
   remote_meas_ready = 0;
   tout = MS250; // zeruj odliczanie czasu od ostatniego rx
   load_lamprom( 1, &lamprem ); // load from EEPROM rather than hold in RAM
   wartmin = 0;
   wartmax = (ELAMP+FLAMP-1);  // wszystkie lampy
   wart = &typ;                               // wskaz na Typ
   EEPROM_READ((int)&poptyp, typ); // ustaw na ostatnio aktywny
   lastyp = typ;
   ADMUX = ADRIH;

   SEI;                                    // wlacz przerwania

//***** Inicjalizacja wyswietlacza LCD ************************

   delay( 30 );                                        // 30ms
   cmd2lcd( 0, 0x28 );
   cmd2lcd( 0, 0x06 );
   cmd2lcd( 0, 0x0c );
   cmd2lcd( 0, 0x01 );
   cmd2lcd( 0, 0x40 );

   cmd2lcd( 0, (0x40 | 0x00) ); for( i = 0; i < 8; i++ ) { cmd2lcd( 1, cyrB[i] ); }
   cmd2lcd( 0, (0x40 | 0x08) ); for( i = 0; i < 8; i++ ) { cmd2lcd( 1, cyrC[i] ); }
   cmd2lcd( 0, (0x40 | 0x10) ); for( i = 0; i < 8; i++ ) { cmd2lcd( 1, cyrD[i] ); }
   cmd2lcd( 0, (0x40 | 0x18) ); for( i = 0; i < 8; i++ ) { cmd2lcd( 1, cyrF[i] ); }
   cmd2lcd( 0, (0x40 | 0x20) ); for( i = 0; i < 8; i++ ) { cmd2lcd( 1, cyrG[i] ); }
   cmd2lcd( 0, (0x40 | 0x28) ); for( i = 0; i < 8; i++ ) { cmd2lcd( 1, cyrI[i] ); }
   cmd2lcd( 0, (0x40 | 0x30) ); for( i = 0; i < 8; i++ ) { cmd2lcd( 1, cyrP[i] ); }
   cmd2lcd( 0, (0x40 | 0x38) ); for( i = 0; i < 8; i++ ) { cmd2lcd( 1, cyrZ[i] ); }

   gotoxy( 0, 0 ); cstr2lcd( 0, "   VTTester 2.06    " );
   gotoxy( 0, 1 ); cstr2lcd( 0, "Tomasz|Adam |       " );
   gotoxy( 0, 2 ); cstr2lcd( 0, "Gumny |Tatus|       " );
   gotoxy( 0, 3 ); cstr2lcd( 0, "forum-trioda.pl/    " );

   for( i = 0; i < 20; i++ ) { WDR; if( i == 19 ) { SPKON; } delay( 100 ); SPKOFF; };

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
      WDR;
      if ( sync == 1 )
      {
         sync = 0;

//***** VTTester 8-byte protocol (power supply typ==0, remote typ==1 only) ***
         /* Parser sets parsed.err_code (OK, OUT_OF_RANGE, CRC, INVALID_CMD, UNKNOWN); we send it + error_param/error_value for SET. */
         if( typ <= 1 )
         {
            if( rx_proto_ready )
            {
               vttester_parsed_t parsed;
               unsigned char cmd, i;
               rx_proto_ready = 0;
               cmd = vttester_parse_message( rx_proto_buf, &parsed );
               if( cmd == VTTESTER_CMD_SET )
               {
                  if( parsed.set.error_param == 0 && typ == 1 )
                  {
                     lamprem.uhdef = parsed.set.uhdef;
                     lamprem.ihdef = parsed.set.ihdef;
                     lamprem.ug1def = parsed.set.ug1def;
                     lamprem.uadef = parsed.set.uadef;
                     lamprem.ug2def = parsed.set.ug2def;
                     uhset = parsed.set.uhdef;
                     ihset = parsed.set.ihdef;
                     ug1set = ug1ref = liczug1( parsed.set.ug1def );
                     uaset = parsed.set.uadef;
                     ug2set = parsed.set.ug2def;
                     tuh = (unsigned int)parsed.set.tuh_ticks * 240u / 60u;
                     lamptem = lamprem;
                  }
                  vttester_send_response( tx_proto_buf, parsed.index,
                     parsed.err_code, parsed.set.error_param, parsed.set.error_value );
                  for( i = 0; i < VTTESTER_FRAME_LEN; i++ ) char2rs( tx_proto_buf[i] );
               }
               else if( cmd == VTTESTER_CMD_MEAS )
               {
                  vttester_send_response( tx_proto_buf, parsed.index, VTTESTER_ERR_OK, 0, 0 );
                  for( i = 0; i < VTTESTER_FRAME_LEN; i++ ) char2rs( tx_proto_buf[i] );
                  remote_meas_index = parsed.index;
                  remote_meas_pending = 1;
                  remote_meas_trigger = 1;
               }
               else
               {
                  /* cmd == VTTESTER_CMD_NONE: CRC, invalid cmd, or unknown */
                  vttester_send_response( tx_proto_buf, parsed.index, parsed.err_code, 0, 0 );
                  for( i = 0; i < VTTESTER_FRAME_LEN; i++ ) char2rs( tx_proto_buf[i] );
               }
            }
            if( remote_meas_ready )
            {
               unsigned char alarm_bits = 0, i;
               if( err & OVERIH ) alarm_bits |= VTTESTER_ALARM_OVERIH;
               if( err & OVERIA ) alarm_bits |= VTTESTER_ALARM_OVERIA;
               if( err & OVERIG ) alarm_bits |= VTTESTER_ALARM_OVERIG;
               if( err & OVERTE ) alarm_bits |= VTTESTER_ALARM_OVERTE;
               vttester_send_measurement( tx_proto_buf, remote_meas_index, ihlcd, ialcd, rangelcd, ig2lcd, slcd, alarm_bits );
               for( i = 0; i < VTTESTER_FRAME_LEN; i++ ) char2rs( tx_proto_buf[i] );
               remote_meas_ready = 0;
               remote_meas_pending = 0;
            }
         }
         else
         {
            /* Local mode (typ > 1): ignore protocol; discard any received 8-byte frame */
            if( rx_proto_ready ) rx_proto_ready = 0;
         }

//***** Wyswietlenie zawartosci bufora ************************

         gotoxy( 0, 0 );                              // Numer
         str2lcd( ((adr == 0) && (start != (FUH+2))), &buf[0] );

         gotoxy( 3, 0 );                              // Nazwa
         char2lcd( 0, ' ' );

         for( i = 0; i < 9; i++ ) char2lcd( (((adr-1) == i) || ((i == 7) && (start == (FUH+2)) && ((buf[11] == '1') || (buf[11] == '2')) )), buf[i+4] );
         gotoxy( 13, 0 );
         cstr2lcd( 0, " G" );
         char2lcd( (adr == 10), '-' );
         str2lcd( (adr == 10), &buf[24] );                // Ug1

         gotoxy( 0, 1 );
         cstr2lcd( 0, "H=" );
         str2lcd( (adr == 11), &buf[14] );                 // Uh
         cstr2lcd( 0, "V A=" );
         str2lcd( (adr == 13), &buf[29] );                 // Ua
         cstr2lcd( 0, " G2=" );
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
         cstr2lcd( 0, "mA " );
         str2lcd( (adr == 14), &buf[33] );                 // Ia
         char2lcd( 0, ' ' );
         str2lcd( (adr == 16), &buf[43] );                // Ig2

         gotoxy( 0, 3 );
         if( typ > 1 )
         {
            gotoxy( 0, 3 );
            cstr2lcd( 0, "S=" );
            str2lcd( (adr == 17), &buf[49] );                 // S

            gotoxy( 6, 3 );
            cstr2lcd( 0, " R=" );
            str2lcd( (adr == 18), &buf[54] );                 // R

            gotoxy( 13, 3 );
            if( (start <= (FUH+2)) || (dusk0 == DMAX) )
            {
               cstr2lcd( 0, " K=" );
               str2lcd( (adr == 19), &buf[59] );                 // K
            }
            else
            {
               cstr2lcd( 0, " T=" );
               int2asc( start >> 2 );

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
            if( typ == 0 ) { cstr2lcd( 0, "                    " ); } else { cstr2lcd( 0, " TX/RX: 9600,8,N,1  " ); }
         }
      }
      GICR = BIT(INT1);                             // wlacz INT1
      if( (err != 0) && ((start == 0) || (start > (TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2))) ) { stop = 1; start = (TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2); } // awaryjne wylaczenie

//***** Wyswietlanie Numeru ***********************************
      if( adr == 0 )                      // ustawianie numeru
      {
         int2asc( typ );
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
               load_lamprom( typ, &lamptem );
               if( (typ != lastyp) && (start != (FUH+2)) ) // zmiana typu, poza restartem
               {
                 load_lamprom( 1, &lamprem );
                 anode = 28;
                 ug1set = ug1zer = ug1ref = liczug1( 240 );    // -24.0V
                 uhset = ihset = uaset = ug2set = 0;
               }
            }
            tuh = (lamptem.nazwa[8] - '0') * 240;    // 240 = 1min
            for( i = 0; i < 9; i++ ) buf[i+4] = (unsigned char)lamptem.nazwa[i];
         }
         else
         {
            EEPROM_READ((int)&lampeep[typ-FLAMP], lamptem);
            tuh = (lamptem.nazwa[8] - 27) * 240;     // 240 = 1min
            for( i = 0; i < 9; i++ ) buf[i+4] = AZ[(unsigned char)lamptem.nazwa[i]];
         }
         lastyp = typ;                           //zapamietaj ostatnio pobrany typ
         nowa = 1;                     // zaktualizowano nazwe
      }

      if( (typ == 0) || (typ == 1) )
      {
         anode = lamptem.nazwa[7];
         buf[11] = AZ[anode];
      }

//***** Edycja Nazwy ******************************************
      if( typ >= FLAMP )
      {
         if( (adr > 0) && (adr < 10) )            // edycja nazwy
         {
//***** Wyswietlanie edytowanej Nazwy *************************
            if( czytaj == 1 ) EEPROM_READ((int)&lampeep[typ-FLAMP].nazwa, lamptem.nazwa);
            for( i = 0; i < 9; i++ ) buf[i+4] = AZ[(unsigned char)lamptem.nazwa[i]];
            czytaj = 0;
            if( dusk0 == DMAX ) zapisz = 1;
            if( (nodus == DMIN) && (zapisz == 1) )
            {
               EEPROM_WRITE((int)&lampeep[typ-FLAMP].nazwa[adr-1], lamptem.nazwa[adr-1]);
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
      ug1 = (unsigned int)licz;
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
      int2asc( (unsigned int)licz );
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
      uh = (unsigned int)licz;
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
      int2asc( (unsigned int)licz );
      buf[14] = (ascii[2] != '0')? ascii[2]: ' ';
      buf[15] = ascii[1];
      buf[16] = '.';
      buf[17] = ascii[0];
//***** Ustawianie Ih *****************************************
      licz = mihadc;
      licz *= vref;
      licz >>= 15;    //   /= 32768;                  // 0..250
      ih = (unsigned int)licz;
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
      int2asc( (unsigned int)licz );
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
      ua = (unsigned int)licz;
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
      int2asc( (unsigned int)licz );
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
      ia = (unsigned int)licz;
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
      int2asc( (unsigned int)licz );
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
      ug2 = (unsigned int)licz;
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
      int2asc( (unsigned int)licz );
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
      ig2 = (unsigned int)licz;
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
      int2asc( (unsigned int)licz );
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
      int2asc( licz );
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
      int2asc( licz );
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
      int2asc( licz );

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
}

