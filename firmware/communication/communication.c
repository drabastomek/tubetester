#include "communication.h"
#include "config/config.h"

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