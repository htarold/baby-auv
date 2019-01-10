/* (C) 2019 Harold Tay GPLv3 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "time.h"
#include "tude.h"
#include "tx.h"
#include "syslog.h"


/*
  Log i2c attitude sensor, see if it bombs over a long period.
 */

const char ident[] PROGMEM = __FILE__ " " __DATE__ " " __TIME__;

void panic(void)
{
  tx_puts("\r\nStopping.\r\n");
  for( ; ; );
}

#define MSG(strlit,v) { tx_msg(strlit ": ", v);syslog_attr(strlit, v); }

#define TEST_PROFILE(arg) \
{ uint16_t s, e; tx_puts(#arg); sleep_mode(); s = TCNT1; arg; \
e = TCNT1; MSG("elapsed", e - s); }

int
main(void)
{
  int8_t ea, em, er, i;

  tx_init();
  sei();
  time_init();
  tx_puts("\r\nStarting\r\n");
  er = syslog_init((char *)ident, SYSLOG_DONT_COPY_TO_SERIAL);
  MSG("syslog_init", er);
  if (er) panic();

again:
  for (i = 4; i > 0; i--) {
    tx_msg("Delay ", i);
    _delay_ms(1000);
  }
  em = ea = 0;

  ea = accel_init();
  MSG("accel_init", ea);
  if (ea) goto again;

  _delay_ms(1000);
  em = cmpas_init();
  MSG("cmpas_init", em);
  if (em) goto again;


  for( ; ; ){
    int16_t angles[3], a[3], m[3];

    ea = accel_read_raw(a);
    if (ea) break;

    MSG("raw_ax", a[0]);
    MSG("raw_ay", a[1]);
    MSG("raw_az", a[2]);

    ea = accel_read(a);
    if (ea) break;

    MSG("ax", a[0]);
    MSG("ay", a[1]);
    MSG("az", a[2]);

    em = cmpas_read_raw(m);
    if (em) break;

    MSG("raw_mx", m[0]);
    MSG("raw_my", m[1]);
    MSG("raw_mz", m[2]);

    em = cmpas_read(m);
    if (em) break;

    MSG("mx", m[0]);
    MSG("my", m[1]);
    MSG("mz", m[2]);
#if 0
    TEST_PROFILE(tude(angles, a, m));

    MSG("spitch", angles[0]);
    MSG("  roll", angles[1]);
    MSG("   yaw", angles[2]);
#endif
    _delay_ms(1000);
  }
  MSG("accel_read_error", ea);
  MSG("cmpas_read_error", em);
  goto again;
  panic();
}
