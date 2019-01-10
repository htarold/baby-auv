/* (C) 2019 Harold Tay GPLv3 */
#include "i2c.h"
#include "tx.h"
#include <avr/interrupt.h>
#include <util/delay.h>
#include "lcd1602.h"
#include "rx.h"
/*
  Display lat/lon in decimal degrees, down to 0.0001 degrees
  (= 11m resolution):
  First line shows target lat/lon like this:
  [-]TTdddd<sp>[-]NNNdddd
  Second line shows age/azimuth/range like this:
  sss<s> NNE mmm<m>
  sss is the age of the most recent update.
  mmm is the range in metres.
 */

/*
  Use I2C address 0x4E/0x4F.
  Backpack uses (LSB) RS RW E x D4 D5 D6 D7 (MSB).
  Backpack uses (MSB) D7 D6 D5 D4 x E RW RS (LSB) 
 */

#define RS 0
#define RW 1
#define EN 2
#define BL 3    /* Backlight */
#define D4 4
#define D5 5
#define D6 6
#define D7 7


void send(uint8_t d)
{
  i2c_start();
  i2c_out(0x4e);
  i2c_out(d);
  i2c_stop();
}
void pulse(uint8_t d)
{
  d |= _BV(BL);                    /* XXX Always on? */
  d |=  _BV(EN); send(d);
  _delay_us(1);
  d &= ~_BV(EN); send(d);
}


#if 0
#define PULSE(d) \
do { tx_puts("\r\n0x"); tx_puthex(d); pulse(d); wait_key(); } while (0)
static void wait_key(void)
{
  while (cbuf_nochar(rx_cbuf)) ;
  (void)cbuf_get(rx_cbuf);
}
#else
#define PULSE(d) pulse(d)
#endif
void init(void)
{
  /* Follows procedure on page 42 */
  _delay_ms(40);
  PULSE(_BV(D5) | _BV(D4));
  _delay_ms(5);
  PULSE(_BV(D5) | _BV(D4));
  _delay_us(100);
  PULSE(_BV(D5) | _BV(D4));
  PULSE(_BV(D5));  /* 4-bit mode */
  PULSE(_BV(D5)); PULSE(_BV(D7)); /* 2 lines */
  PULSE(0); PULSE(_BV(D7)); /* Display off */
  PULSE(0); PULSE(_BV(D6) | _BV(D5));  /* I/D, S */

  PULSE(0); PULSE(_BV(D7) | _BV(D6) | _BV(D5)); /* Display on */
  PULSE(0); PULSE(_BV(D6) | _BV(D5)); /* Entry mode set */
}

uint8_t column;
void put(uint8_t ch)
{
  PULSE(_BV(RS) | (ch&0xf0));
  PULSE(_BV(RS) | (ch<<4));
  column++;
}
void row(uint8_t which)  /* 0 -> top row, 1 -> bottom row */
{
  uint8_t d;
  d = _BV(D7);
  if (which) d |= _BV(D6);
  PULSE(d);
  PULSE(0);
  column = 0;
}
void cleartocol(uint8_t col)
{
  while (column < col) put(' ');
}
#define cleartoeol() cleartocol(15)

int
main(void)
{
  int8_t i;
  sei();
  tx_init();
  rx_init();
  rx_enable();

  for (i = 4; i > 0; i--) {
    tx_puts("\r\nDelay ");
    tx_putdec(i);
    _delay_ms(1000);
  }
  i2c_init();
  init();

  tx_puts("lcd_init complete\r\n");
  _delay_ms(1000);

  for (i = 0; i < 4; i++) {
    put("Test\n"[i]);
    _delay_ms(200);
  }
  tx_puts("Done\r\n");
  for ( ; ; );
}
