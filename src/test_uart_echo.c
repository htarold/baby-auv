/* (C) 2019 Harold Tay GPLv3 */
/*
  Data from host is echoed.
 */

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "tx.h"
#include "rx.h"

int
main(void)
{
  tx_init();
  rx_init();
  rx_enable();
  sei();
  tx_msg("Starting: ", 1);

  for( ; ; ){
    char ch;
    if (rx_havechar()){
      ch = rx_getchar();
      /* _delay_ms(20); */
      tx_putc(ch);
      if (ch == '\r')
        tx_putc('\n');
    }
  }
}
