#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#define P10_A       (1 << PB0)
#define P10_B       (1 << PB1)
#define P10_R       (1 << PB2)
#define P10_CLK     (1 << PB3)
#define P10_STR     (1 << PB4)

#define F_CPU       8000000UL
#define BAUD_RATE   9600UL
#define UBRR_VAL  (F_CPU / (16 * BAUD_RATE) - 1)

#define enable_usart_rx_int()   (UCSRB |= (1 << RXCIE))
#define disable_usart_rx_int()  (UCSRB &= ~(1 << RXCIE))
#define enable_usart_tx_int()   (UCSRB |= (1 << TXCIE))
#define disable_usart_tx_int()  (UCSRB &= ~(1 << TXCIE))

#define BUFF_SIZE         5
#define SYM_COUNT         4
#define GROUP_COUNT       4
#define BYTES_IN_GROUP    4
#define BYTES_IN_SYMBOL   16

void p10_print_current_symbols();
static uint8_t current_symbols[BUFF_SIZE] = {0};
//////////////////////////////////////////////////////////////

static volatile uint8_t rx_n = 0;
ISR(USART_RX_vect) {
  register uint8_t udr_val;
  disable_usart_rx_int();
  udr_val = UDR;
  if (udr_val > 9)
    udr_val = 9;
  current_symbols[rx_n] = udr_val;
  if (++rx_n == BUFF_SIZE)
    rx_n = 0;
  enable_usart_rx_int();
}
//////////////////////////////////////////////////////////////

ISR(USART_TX_vect) {
  disable_usart_tx_int();
  enable_usart_tx_int();
}
//////////////////////////////////////////////////////////////

int
main(void) {
  DDRB =  P10_A   | P10_B |
          P10_CLK | P10_STR | P10_R;
  PORTB = 0x00;
  UCSRC = (1 << UCSZ0) | (1 << UCSZ1); //8bit, 1 stop, async
  UBRRH = (UBRR_VAL >> 8) & 0xff;
  UBRRL = UBRR_VAL & 0xff;
  UCSRB = (1 << RXEN) | (1 << TXEN);
  enable_usart_rx_int();
  enable_usart_tx_int();
  sei();
  while (1) {
    p10_print_current_symbols();
  }
  return 0;
}
//////////////////////////////////////////////////////////////

inline void set_group(int8_t group) {
  switch (group) {
    case 0x00: //up
      PORTB &= ~P10_A;
      PORTB &= ~P10_B;
      break;
    case 0x01:
      PORTB |= P10_A;
      PORTB &= ~P10_B;
      break;
    case 0x02:
      PORTB &= ~P10_A;
      PORTB |= P10_B;
      break;
    case 0x03: //down
      PORTB |= P10_A;
      PORTB |= P10_B;
      break;
  }
}
//////////////////////////////////////////////////////////////

static const uint8_t sym_table[] PROGMEM= {
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

void
p10_print_current_symbols() {
  //group, group_byte, byte_bit, current_symbol, temp
  int8_t gr, gb, bb, cs, tmp;

  for (gr = 0; gr < GROUP_COUNT; ++gr) {
    set_group(gr);
    for (cs = 0; cs < SYM_COUNT; ++cs) {
      for (gb = 0; gb < BYTES_IN_GROUP; ++gb) {

        tmp = pgm_read_byte(&sym_table[current_symbols[cs]*BYTES_IN_GROUP*GROUP_COUNT + gr*BYTES_IN_GROUP + gb]);

        if (current_symbols[BUFF_SIZE-1]) {
          if (cs == 2) {
            if (gb == 1 && (gr == 1 || gr == 2)) tmp |= 0b10000000;
            if (gb == 2 && (gr == 0 || gr == 3)) tmp |= 0b10000000;
          }
        }

        for (bb = 7; bb >=0 ; --bb) {
          if (tmp & (1 << bb)) PORTB &= ~P10_R;
          else PORTB |= P10_R;

          PORTB |= P10_CLK;
          PORTB &= ~P10_CLK;
        } //for j
      } //for i

    } //for k
    PORTB |= P10_STR;
    PORTB &= ~P10_STR;
  } //for group
}
//////////////////////////////////////////////////////////////
