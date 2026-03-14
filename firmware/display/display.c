#include "display.h"
#include "config/config.h"
#include "utils/utils.h"
#include "communication/communication.h"

#include <util/delay.h>

// MINIMAL globals here
uint8_t
   ascii[5],
   bufin[10],
   buf[64];

void lcd_pulse_enable(void)
{
	PORTC |=  (1 << LCD_E);
	_delay_us(1);
	PORTC &= ~(1 << LCD_E);
	_delay_us(100);
}

void lcd_write_nibble(uint8_t nibble)
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
   /* ICCAVR cursor: when field is highlighted and takt==0, show space so highlighted field blinks */
   cmd2lcd(1, ((highlight == 1) && (takt == 0)) ? ' ' : c);
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

void lcd_init(void)
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
      char2lcd( f, ch );
      c++;
   }
}

void int2asc( uint16_t liczba, uint8_t *ascii)
{
   uint8_t i;
   uint8_t temp;

   for( i = 0; i < 4; i++ )
   {
      temp = (uint8_t)(liczba % 10);
      liczba /= 10;
      ascii[i] = '0' + temp;
   }
}

void send_cyrillic_chars(void) {
   cmd2lcd( 0, (0x40 | 0x00) ); for( i = 0; i < 8; i++ ) { cmd2lcd( 1, (uint8_t)cyrB[i] ); }
   cmd2lcd( 0, (0x40 | 0x08) ); for( i = 0; i < 8; i++ ) { cmd2lcd( 1, (uint8_t)cyrC[i] ); }
   cmd2lcd( 0, (0x40 | 0x10) ); for( i = 0; i < 8; i++ ) { cmd2lcd( 1, (uint8_t)cyrD[i] ); }
   cmd2lcd( 0, (0x40 | 0x18) ); for( i = 0; i < 8; i++ ) { cmd2lcd( 1, (uint8_t)cyrF[i] ); }
   cmd2lcd( 0, (0x40 | 0x20) ); for( i = 0; i < 8; i++ ) { cmd2lcd( 1, (uint8_t)cyrG[i] ); }
   cmd2lcd( 0, (0x40 | 0x28) ); for( i = 0; i < 8; i++ ) { cmd2lcd( 1, (uint8_t)cyrI[i] ); }
   cmd2lcd( 0, (0x40 | 0x30) ); for( i = 0; i < 8; i++ ) { cmd2lcd( 1, (uint8_t)cyrP[i] ); }
   cmd2lcd( 0, (0x40 | 0x38) ); for( i = 0; i < 8; i++ ) { cmd2lcd( 1, (uint8_t)cyrZ[i] ); }
}

void show_splash(void) {
   gotoxy( 0, 0 ); cstr2lcd_P( 0, PSTR("   VTTester 2.06    ") );
   gotoxy( 0, 1 ); cstr2lcd_P( 0, PSTR("Tomasz|Adam |       ") );
   gotoxy( 0, 2 ); cstr2lcd_P( 0, PSTR("Gumny |Tatus|       ") );
   gotoxy( 0, 3 ); cstr2lcd_P( 0, PSTR("forum-trioda.pl/    ") );
}

void update_display(void) {
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

   prepare_values_to_display();
}

void prepare_values_to_display(void) {
   update_name_and_number();
   update_ug1();
   update_uh();
   update_ih();
   update_ua();
   update_ia();
   update_ug2();
   update_ig2();
   update_s();
   update_r();
   update_k();
}

void update_name_and_number(void) {
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
               memcpy_P(&lamprem, &lamprom[1], sizeof(lamprem)); // kazdy inny kasuje dane remote
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
}

//***** Ustawianie Ug1 ****************************************
void update_ug1(void) {
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
}

//***** Ustawianie Uh *****************************************
void update_uh(void) {
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
}

//***** Ustawianie Ih *****************************************
void update_ih(void) {
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
}

//***** Ustawianie Ua *****************************************
void update_ua(void) {
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
}

//***** Ustawianie Ia ***************************************
void update_ia(void) {
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
}

//***** Ustawianie Ug2 ****************************************
void update_ug2(void) {
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
}

//***** Ustawianie Ig2 **************************************
void update_ig2(void) {
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
}

//***** Ustawianie S ****************************************
void update_s(void) {
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
}

//***** Ustawianie R ****************************************
void update_r(void) {
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
}

//***** Ustawianie K ****************************************
void update_k(void) {
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
}

void send_data_to_pc(void) {
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
