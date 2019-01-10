/* (C) 2019 Harold Tay LGPLv3 */
/*
  Scan all addresses.
 */

#include "i2c.h"
#include "tx.h"
/* #include "ctd.h" */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

static void say(char * msg, int8_t er)
{
  tx_puts(msg);
  tx_puts(": ");
  tx_putdec(er);
}
static void bomb(char * msg, int8_t er)
{
  tx_strlit("\r\nError ");
  say(msg, er);
  for ( ; ; );
}

void syslog_attr(char * unused, int16_t unused2) {}
void syslog_attrpgm(char * unused, int16_t unused2) {}
void syslog_lattr(char * unused, int32_t unused2) {}

int
main(void)
{
  int8_t i, er;

  tx_init();
  sei();

  for (i = 4; i > 0; i--) {
    tx_strlit("\r\nDelay ");
    tx_putdec(i);
    _delay_ms(1000);
  }

  /* ctd_start(); */
  _delay_ms(500);
  i2c_init();

  for (i = 1; i < 127; i++) {
    er = i2c_start();
    if (er) bomb("i2c_start", er);
    tx_strlit("\r\naddress: 0x");
    tx_puthex(i);
    er = i2c_out(i<<1);
    if (er != TW_MT_SLA_ACK)
      say(" Error ", er);
    else
      tx_puts(" Acked");
    i2c_stop();
    _delay_ms(10);
  }

  for ( ; ; );
}
