#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>

typedef struct screen_operation {
  void (*clr)();
  void (*set_pixel)(uint8_t x, uint8_t y);
  void (*clr_pixel)(uint8_t x, uint8_t y);
  void (*show)();
} screen_op_t;
//////////////////////////////////////////////////////////////

screen_op_t*
get_screen_operations();

#endif // SCREEN_H
