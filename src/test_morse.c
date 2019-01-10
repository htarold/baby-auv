/* (C) 2019 Harold Tay GPLv3 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "time.h"
#include "morse.h"
#include "tx.h"


void range(char start, char end)
{
  char ch;
  for(ch = start; ch <= end; ch++) {
    morse_putc(ch);
    tx_putc(ch);
    while (!morse_is_idle()) ;
  }
}

int
main(void)
{
  _delay_ms(2000);
  tx_init();
  sei();
  time_init();
  morse_init();
  tx_puts("\r\nStarting\r\n");

  range('a', 'z');
  tx_puts("\r\n");
  range('A', 'Z');
  tx_puts("\r\n");
  range('0', '9');
  tx_puts("\r\n");

  for( ; ; );
}
