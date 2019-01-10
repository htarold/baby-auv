/* (C) 2019 Harold Tay GPLv3 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "tx.h"
#include "fmt.h"
#include "time.h"
#include "syslog.h"
#include "mma.h"
#include "tude.h"
#include "pitch.h"

const char ident[] PROGMEM = __FILE__ " " __DATE__ " " __TIME__ ;

struct angles angles;

void delay(void)
{
  time_delay(100);
  tude_get(&angles, TUDE_RATE_FAST);
  time_delay(100);
  tude_get(&angles, TUDE_RATE_FAST);
  time_delay(100);
  tude_get(&angles, TUDE_RATE_FAST);
  time_delay(100);
  tude_get(&angles, TUDE_RATE_FAST);
  time_delay(100);
  tude_get(&angles, TUDE_RATE_FAST);
  time_delay(100);
  tude_get(&angles, TUDE_RATE_FAST);
  time_delay(100);
  tude_get(&angles, TUDE_RATE_FAST);
  time_delay(100);
  tude_get(&angles, TUDE_RATE_FAST);
  time_delay(100);
  tude_get(&angles, TUDE_RATE_FAST);
  time_delay(100);
  tude_get(&angles, TUDE_RATE_FAST);
  time_delay(100);
  tude_get(&angles, TUDE_RATE_FAST);
  time_delay(100);
  tude_get(&angles, TUDE_RATE_FAST);
  time_delay(100);
  tude_get(&angles, TUDE_RATE_FAST);
}

static void waitforpitchup(void)
{
  for( ; ; ) {
    tude_get(&angles, TUDE_RATE_FAST);
    if (angles.sin_pitch > 707) break; /* about 45 degrees */
    time_delay(100);
  }
}

int main(void)
{
  int8_t er, i;

  tx_init();
  time_init();
  sei();
  for(i = 0; i < 4; i++) {
    tx_msg("Delay ", 4-i);
    _delay_ms(1000);
  }
  er = syslog_init((char *)ident, SYSLOG_DO_COPY_TO_SERIAL);
  if (er) {
    tx_msg("syslog_init = ", er);
    time_delay(100);
    er = syslog_init((char *)ident, 1);
  }
  if (er) {
    tx_msg("syslog_init = ", er);
    goto done;
  }

  accel_init();
  cmpas_init();

  pitch_init();
  time_delay(100);

  (void)pitch_up();
  waitforpitchup();

  tx_puts("Ready!\r\n");
  time_delay(100);  /* 1 second */

  for (i = 0; i < 20; i++) {
    syslog_puts("Nose Level\n");
    (void)pitch_level();
    delay();
    syslog_puts("Nose Up\n");
    (void)pitch_up();
    waitforpitchup();
    delay();
  }
done:
  for( ; ; ) {
    syslog_puts("Stopping\r\n");
    time_delay(50);
  }
}
