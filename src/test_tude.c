/* (C) 2019 Harold Tay GPLv3 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include "time.h"
#include "tude.h"
#include "tx.h"

void panic(void)
{
  tx_puts("\r\nStopping.\r\n");
  for( ; ; );
}

#define TEST_PROFILE(arg) \
{ uint16_t s, e; sleep_mode(); s = TCNT1; arg; \
e = TCNT1; tx_puts(#arg); tx_msg(" elapsed = ", e - s); }

int
main(void)
{
  int8_t ea, em;

  tx_init();
  sei();
  time_init();
  tx_puts("\r\nStarting\r\n");

  em = ea = 0;

  ea = accel_init();
  tx_msg("accel_init = ", ea);
  if (ea) { panic(); }

  _delay_ms(1000);
  em = cmpas_init();
  tx_msg("cmpas_init = ", em);
  if (em) { panic(); }

  for( ; ; ){
    struct angles t;
#if 1
    int16_t a[3], m[3];

    ea = accel_read_raw(a);
    if (ea) break;

    tx_msg("raw_ax:", a[0]);
    tx_msg("raw_ay:", a[1]);
    tx_msg("raw_az:", a[2]);

    ea = accel_read(a);
    if (ea) break;

    tx_msg("ax:", a[0]);
    tx_msg("ay:", a[1]);
    tx_msg("az:", a[2]);

    em = cmpas_read_raw(m);
    if (em) break;

    tx_msg("raw_mx:", m[0]);
    tx_msg("raw_my:", m[1]);
    tx_msg("raw_mz:", m[2]);

    em = cmpas_read(m);
    if (em) break;

    tx_msg("mx:", m[0]);
    tx_msg("my:", m[1]);
    tx_msg("mz:", m[2]);

    TEST_PROFILE(tude_read(&t));
#else
    /*
      Approx 2.9ms per call at 100kHz I2C speed.
      Approx 2.0ms per call at 200kHz.  This I2C bus cannot go
      much higher than 200kHz without stronger pullups.
     */
    TEST_PROFILE(tude_read(&t));
#endif

    tx_msg("spitch:", t.sin_pitch);
    tx_msg("  roll:", t.roll);
    tx_msg("   yaw:", t.heading);

    _delay_ms(1000);
  }
  tx_msg("accel_read error: ", ea);
  tx_msg("cmpas_read error: ", em);
  panic();
}
