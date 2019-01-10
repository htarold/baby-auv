/* (C) 2019 Harold Tay GPLv3 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include "tx.h"
#include "spt.h"
#include "cbuf.h"
#include "time.h"

void tick(void)
{
  static uint8_t h;
  for ( ; h == time_uptimeh; ) {
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_mode();
  }
  h = time_uptimeh;
}

char token[32];
int8_t scan(void)
{
  static int8_t len = 0;
  int8_t ret;

  for ( ; ; ) {
    char ch;
    if (cbuf_nochar(spt_rx)) return(0);
    ch = cbuf_get(spt_rx);
    token[len++] = ch;
    if (len == sizeof(token)-1) break;
    if (ch == ',') break;
    if (ch == '*') break;
    if (ch == '\r') break;
    if (ch == '\n') break;
  }
  token[len] = '\0';
  ret = len; len = 0;
  return(ret);
}

int
main(void)
{
  int8_t i;

  /*
    Debug
   */
  DDRC |= _BV(PC3);

  /*
    Turn on GPS
   */
  DDRD |= _BV(PD6) | _BV(PD7);
  PORTD |= _BV(PD7);                  /* gp_sel_b */
  PORTD &= ~_BV(PD6);                 /* gp_sel_a */

  tx_init();
  time_init();
  sei();

  for (i = 4; i > 0; i--) {
    _delay_ms(1000);
    tx_puts("Delay ");
    tx_putdec(i);
    tx_puts("\r\n");
  }

  spt_set_speed_9600();
  spt_rx_start();

  for ( ; ; ) {
    int8_t len;
    tick();
    len = scan();
    if (len <= 0) continue;
    tx_putc('>');
    tx_puts(token);
    tx_putc('<');
  }
}
