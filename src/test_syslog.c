/* (C) 2019 Harold Tay GPLv3 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "syslog.h"
#include "time.h"
#include "tx.h"
#include "rx.h"
#include "morse.h"

/*
  Everything is printable from space (32) to ~ 126
 */

const char ident[] PROGMEM = __FILE__ " " __DATE__ " " __TIME__;

char * chars =
"abcdefghijklmnopqrstuvwxyz"
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"1234567890+="
"abcdefghijklmnopqrstuvwxyz"
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"1234567890+=";

int
main(void)
{
  uint32_t bytes_written;
  uint16_t i;
  int8_t er;

  tx_init();
  time_init();
  rx_init();
  rx_enable();
  sei();
  morse_init();
  er = syslog_init(ident, SYSLOG_DO_COPY_TO_SERIAL);
  tx_msg("syslog_init(): ", er);
  if (er) {
    for( ; ; );
  }
  for(i = 0; i < 4; i++) {
    tx_msg("Delay ", 3-i);
    _delay_ms(1000);
  }
  tx_puts("Starting\r\n");

#if 0
  for( ; ; ){
    char ch;
    if (!rx_havechar()) continue;
    ch = rx_getchar();
    if (ch <= 32)continue;
    for(i = 0; i < 63; i++) {
      syslog_putc(ch++);
      if (ch > 126) ch = 33;
    }
    syslog_putc('\n');
  }
#else
  bytes_written = 0UL;
  for(i = 0; i < 4096; i++) {
    static uint8_t start = 0;
    uint8_t rndlen;
    while (!(rndlen = TCNT1 & 63));
    start %= 64;
    syslog_put(chars + start, rndlen);
    start += rndlen;
    bytes_written += rndlen;
    start %= 64;
    syslog_putc('\n');
    tx_msg("line ", i);
    if (syslog_error) break;
    _delay_ms(80);
  }
#endif
  tx_msg("syslog_error: ", syslog_error);
  tx_putdec32(bytes_written); tx_puts(" bytes written\r\n");
  tx_puts("\r\nStopping\r\n");
  for( ; ; );
}
