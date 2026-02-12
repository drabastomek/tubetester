/*
 * Helper functions (FUNKCJE POMOCNICZE) - UART, delay, zero S/R/K, Ug1 conversion.
 */

#ifndef VTTESTER_UTILS_H
#define VTTESTER_UTILS_H

void char2rs(unsigned char bajt);
void cstr2rs(const char *q);
void delay(unsigned char opoz);
void zersrk(void);
unsigned int liczug1(unsigned int pug1);
/* Write 4 decimal digits (units..thousands) into ascii[0..3]. */
void int2asc(unsigned int liczba, unsigned char *ascii);

#endif /* VTTESTER_UTILS_H */
