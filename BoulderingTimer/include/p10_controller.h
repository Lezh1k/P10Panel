#ifndef P10_CONTROLLER_H
#define P10_CONTROLLER_H

#include <stdint.h>

class QSerialPort;

class CP10Controller {

public:
  static const int BYTES_IN_GROUP = 4;
  static const int GROUP_CNT = 4;
  static const int BITS_COUNT = 8; //in byte on AVR and Intel
  static const int COL_CNT = 4;
  static const int ROW_CNT = 16;
  static const int BUFF_SIZE = (COL_CNT*ROW_CNT);

  void clr();
  int set_pixel(uint32_t x, uint32_t y);
  int clr_pixel(uint32_t x, uint32_t y);
  void set_digit(uint8_t pos, uint8_t dig);
  void set_serial_port(QSerialPort* port);

  void p10_send_byte(uint8_t ix);

  static CP10Controller* Instance() {
    static CP10Controller instance;
    return &instance;
  }

private:
  QSerialPort* m_serial_port;
  uint8_t m_virtual_screen[BUFF_SIZE] = {0};

  CP10Controller();
  ~CP10Controller();
};

#endif // P10_CONTROLLER_H
