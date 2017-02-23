#include <QSerialPort>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <string.h>
#include <QDebug>

#include "include/p10_controller.h"

CP10Controller::CP10Controller()
{

}

CP10Controller::~CP10Controller()
{

}
//////////////////////////////////////////////////////////////

void
CP10Controller::clr() {
  memset(m_virtual_screen, 0, BUFF_SIZE);
}
//////////////////////////////////////////////////////////////


//todo calculate these values
static const uint8_t yr_vals[CP10Controller::ROW_CNT] = {
  51, 3, 19, 35,
  50, 2, 18, 34,
  49, 1, 17, 33,
  48, 0, 16, 32
};

int CP10Controller::set_pixel(uint32_t x, uint32_t y) {
  uint8_t yr = yr_vals[y];
  uint8_t xr = yr + ((x / BITS_COUNT) * COL_CNT);
  m_virtual_screen[xr] |= (0x80 >> (x % 8));
  return xr;
}
//////////////////////////////////////////////////////////////

int CP10Controller::clr_pixel(uint32_t x, uint32_t y) {
  uint8_t yr = yr_vals[y];
  uint8_t xr = yr + ((x / BITS_COUNT) * COL_CNT);
  m_virtual_screen[xr] &= ~(0x80 >> (x % 8));
  return xr;
}
//////////////////////////////////////////////////////////////

void
CP10Controller::set_digit(uint8_t pos, uint8_t dig) {
  static const uint8_t sym_table[] = {
    //0
    0b01111111, 0b01100011, 0b01100011, 0b01111111,
    0b01111111, 0b01100011, 0b01100011, 0b01111111,
    0b00000000, 0b01100011, 0b01100011, 0b01100011,
    0b01100011, 0b01100011, 0b01100011, 0b00000000,
    //1
    0b00000011, 0b00000011, 0b00000011, 0b00000011,
    0b00000011, 0b00000011, 0b00000011, 0b00000011,
    0b00000000, 0b00000011, 0b00000011, 0b00000011,
    0b00000011, 0b00000011, 0b00000011, 0b00000000,
    //2
    0b01111111, 0b01100000, 0b00000011, 0b01111111,
    0b01111111, 0b01100000, 0b00000011, 0b01111111,
    0b00000000, 0b01100000, 0b01111111, 0b00000011,
    0b01100000, 0b01111111, 0b00000011, 0b00000000,
    //3
    0b01111111, 0b00000011, 0b00000011, 0b01111111,
    0b01111111, 0b00000011, 0b00000011, 0b01111111,
    0b00000000, 0b00000011, 0b01111111, 0b00000011,
    0b00000011, 0b01111111, 0b00000011, 0b00000000,
    //4
    0b00000011, 0b00000011, 0b01100011, 0b01100011,
    0b00000011, 0b00000011, 0b01100011, 0b01100011,
    0b00000000, 0b00000011, 0b01111111, 0b01100011,
    0b00000011, 0b01111111, 0b01100011, 0b00000000,
    //5
    0b01111111, 0b00000011, 0b01100000, 0b01111111,
    0b01111111, 0b00000011, 0b01100000, 0b01111111,
    0b00000000, 0b00000011, 0b01111111, 0b01100000,
    0b00000011, 0b01111111, 0b01100000, 0b00000000,
    //6
    0b01111111, 0b01100011, 0b01100000, 0b01111111,
    0b01111111, 0b01100011, 0b01100000, 0b01111111,
    0b00000000, 0b01100011, 0b01111111, 0b01100000,
    0b01100011, 0b01111111, 0b01100000, 0b00000000,
    //7
    0b00000011, 0b00000011, 0b00000011, 0b01111111,
    0b00000011, 0b00000011, 0b00000011, 0b01111111,
    0b00000000, 0b00000011, 0b00000011, 0b00000011,
    0b00000011, 0b00000011, 0b00000011, 0b00000000,
    //8
    0b01111111, 0b01100011, 0b01100011, 0b01111111,
    0b01111111, 0b01100011, 0b01100011, 0b01111111,
    0b00000000, 0b01100011, 0b01111111, 0b01100011,
    0b01100011, 0b01111111, 0b01100011, 0b00000000,
    //9
    0b01111111, 0b00000011, 0b01100011, 0b01111111,
    0b01111111, 0b00000011, 0b01100011, 0b01111111,
    0b00000000, 0b00000011, 0b01111111, 0b01100011,
    0b00000011, 0b01111111, 0b01100011, 0b00000000,
  };

  for (int gr = 0; gr < GROUP_CNT; ++gr) {
    for (int gb = 0; gb < BYTES_IN_GROUP; ++gb) {
      m_virtual_screen[gr*GROUP_CNT*BYTES_IN_GROUP + pos*BYTES_IN_GROUP + gb] =
          sym_table[dig*GROUP_CNT*BYTES_IN_GROUP + gr*BYTES_IN_GROUP + gb];
      p10_send_byte(gr*GROUP_CNT*BYTES_IN_GROUP + pos*BYTES_IN_GROUP + gb);
    } //for gb
  } //for gr
}
//////////////////////////////////////////////////////////////

void
CP10Controller::set_serial_port(QSerialPort *port) {
  m_serial_port = port;
}
//////////////////////////////////////////////////////////////

void
CP10Controller::p10_send_byte(uint8_t ix) {
  static const uint8_t TX_BUFFER_SIZE = 2;
  static char cmd[TX_BUFFER_SIZE] = {0};
  if (m_serial_port==nullptr || !m_serial_port->isOpen())
    return;
  cmd[0] = ix;
  cmd[1] = m_virtual_screen[ix];
  m_serial_port->write(cmd, TX_BUFFER_SIZE);
  m_serial_port->flush();
}
//////////////////////////////////////////////////////////////
