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
#define UBRR_VAL    (F_CPU / (16 * BAUD_RATE) - 1)

#define enable_usart_rx_int()   (UCSRB |= (1 << RXCIE))
#define disable_usart_rx_int()  (UCSRB &= ~(1 << RXCIE))
#define enable_usart_tx_int()   (UCSRB |= (1 << TXCIE))
#define disable_usart_tx_int()  (UCSRB &= ~(1 << TXCIE))

#define SCREEN_BUFF_SIZE         64
#define SCREEN_BYTES_IN_GROUP    16
#define SCREEN_GROUP_CNT         (SCREEN_BUFF_SIZE / SCREEN_BYTES_IN_GROUP)

#define COMMAND_BUFF_SIZE        2

static uint8_t pixel_buffer[SCREEN_BUFF_SIZE] = {0};
static uint8_t cmd_buffer[COMMAND_BUFF_SIZE] = {0};
static volatile uint8_t cmd_received = 0;

void p10_handle_usart_cmd();
void p10_print_pixel_buffer();
//////////////////////////////////////////////////////////////

static volatile uint8_t rx_n = 0;
ISR(USART_RX_vect) {  
  disable_usart_rx_int();
  cmd_buffer[rx_n] = UDR;
  if (++rx_n == COMMAND_BUFF_SIZE) {
    cmd_received = 1;
    rx_n = 0;
  }
  if (!cmd_received)
    enable_usart_rx_int();
}
//////////////////////////////////////////////////////////////

int
main(void) {
  uint8_t i;
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
    p10_print_pixel_buffer();
  }
  return 0;
}
//////////////////////////////////////////////////////////////

inline void set_group(int8_t group) {
  switch (group) {
    case 0x00:
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
    case 0x03:
      PORTB |= P10_A;
      PORTB |= P10_B;
      break;
  }
}
//////////////////////////////////////////////////////////////

void
p10_handle_usart_cmd() {
  pixel_buffer[cmd_buffer[0]] = cmd_buffer[1];
  enable_usart_rx_int();
}
//////////////////////////////////////////////////////////////

void
p10_print_pixel_buffer() {
  //group, group_byte, byte_bit, current_symbol
  int8_t gr, gb, bb, cs = 0;
  for (gr = 0; gr < SCREEN_GROUP_CNT; ++gr) {
    set_group(gr);
    for (gb = 0; gb < SCREEN_BYTES_IN_GROUP; ++gb, ++cs) {

      if (cmd_received) {
        p10_handle_usart_cmd();
        cmd_received = 0;
      }

      for (bb = 7; bb >=0 ; --bb) {
        if (pixel_buffer[cs] & (1 << bb)) PORTB &= ~P10_R;
        else PORTB |= P10_R;

        PORTB |= P10_CLK;
        PORTB &= ~P10_CLK;
      } //for bb
    } //for gb

    PORTB |= P10_STR;
    PORTB &= ~P10_STR;
  } //for group
}
//////////////////////////////////////////////////////////////
