/* (C) 2019 Harold Tay GPLv3 */
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "time.h"
#include "tx.h"
#include "tude_task.h"
#include "syslog.h"
#include "task.h"


/*
  Use a scheduler, to "multitask".
  Tasks that need to run:
  sensors
  depth
  navigation
  Some functions don't need to be a task, but are called by
  tasks and run synchronously, e.g. tx_*(), syslog*(), morse()?

  Tasks communicate with each other by passing messages.

  The task list is run every 10ms.
 */

/*
  Task to print out state vars
 */
int8_t print_task(void)
{
  static int8_t delay = 0;
  int8_t var, er;

  delay++;
  if (delay < 100) return(0);
  delay = 0;

  er = tude_get_pitch(&var);
  if (er) syslog_puts("tude_get_pitch() returned error\n");
  syslog_attr("theta", var);
  er = tude_get_roll(&var);
  if (er) syslog_puts("tude_get_roll() returned error\n");
  syslog_attr("phi", var);
  er = tude_get_heading(&var);
  if (er) syslog_puts("tude_get_heading() returned error\n");
  syslog_attr("hdg", var);
  return(0);
}

int8_t (*tasks[])(void) = {
  tude_task,
  print_task,
};

const char ident[] PROGMEM = __FILE__ " " __DATE__ " " __TIME__;

int
main(void)
{
  uint8_t i;
  int8_t er;

  tx_init();

  for (i = 4; i > 0; i--) {
    tx_msg("Delay ", i);
    _delay_ms(1000);
  }

  time_init();
  sei();
  syslog_init(ident, SYSLOG_DO_COPY_TO_SERIAL);

  er = tude_init();
  if (er) {
    syslog_puts("tude_init error ");
    syslog_i16d(er);
    syslog_putc('\n');
    for ( ; ; );
  }

  for ( ; ; ) {
    uint8_t huns;

    /*
      We may skip a beat, but will always start at the top of a
      tick.
     */
    for(huns = time_uptimeh; huns == time_uptimeh; ) {
      set_sleep_mode(SLEEP_MODE_IDLE);
      sleep_mode();
    }
    for (i = 0; i < sizeof(tasks)/sizeof(*tasks); i++) {
      er = tasks[i]();
      if (er) {
        syslog_puts("task ");
	syslog_i16d(i);
	syslog_puts(" returned ");
	syslog_i16d(er);
	syslog_putc('\n');
      }
    }
  }
}
