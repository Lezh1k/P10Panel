#ifndef P10_CONTROLLER_H
#define P10_CONTROLLER_H

#include <stdint.h>

class CP10Controller {

private:

public:
  void p10_clr();
  void p10_set_pixel(uint32_t x, uint32_t y);
  void p10_clr_pixel(uint32_t x, uint32_t y);
  void p10_set_digit(uint8_t pos, uint8_t dig);
}

#endif // P10_CONTROLLER_H
