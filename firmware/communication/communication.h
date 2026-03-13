#ifndef COMMUNICATION_H 
#define COMMUNICATION_H

#include <stdint.h>

void send_data(void);
void receive_data(void);

extern void char2rs(uint8_t bajt);
extern void cstr2rs(const char* q);

#endif