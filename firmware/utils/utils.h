/*
 * Helper functions (FUNKCJE POMOCNICZE) - UART, delay, zero S/R/K, Ug1 conversion.
 */

#ifndef VTTESTER_UTILS_H
#define VTTESTER_UTILS_H

#include <stdint.h>

void char2rs(uint8_t bajt);
void cstr2rs(const char *q);
void delay(uint8_t opoz);
void zersrk(void);
uint16_t liczug1(uint16_t pug1);
/* Write 4 decimal digits (units..thousands) into ascii[0..3]. */
void int2asc(uint16_t liczba, uint8_t *ascii);

#endif /* VTTESTER_UTILS_H */
