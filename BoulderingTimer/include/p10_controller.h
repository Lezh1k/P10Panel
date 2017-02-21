#ifndef P10_CONTROLLER_H
#define P10_CONTROLLER_H

#include <stdint.h>

void p10_clr();
void p10_set_pixel(uint8_t x, uint8_t y);
void p10_clr_pixel(uint8_t x, uint8_t y);

#endif // P10_CONTROLLER_H
