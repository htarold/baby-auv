/* (C) 2019 Harold Tay LGPLv3 */
/*
  Separate hardware made to switch 3 gps units with the soft
  port (PD2).
  Not using multitasking.
 */
#include "tx.h"
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "time.h"
#include "sel.h"
#include "cbuf.h"
#include "spt.h"
#include "ub.h"
#include "ppm.h"
#include "syslog.h"
#include "yield.h"

uint8_t fg_buffer[64];

void gps_power_on(void) { ppm_init(); }
void gps_power_off(void) { ppm_stop(); }

void gps_select(int8_t which)
{
  /*
    Don't use SEL_A or SEL_B (used for gps power etc).  Use GPA2
    and GPA3.
    GPA3 -> 4052_A, GPA2 -> 4052_B
    GPA2/GPA3   -> channel/pin -> GPS unit#
    0/0         -> 0Y/pin1     -> 0
    0/1         -> 1Y/pin5     -> not used
    1/0         -> 2Y/pin2     -> 1
    1/1         -> 3Y/pin4     -> 2
   */
  if (0 == which) {
    PORTC &= ~_BV(PC2);
    PORTC &= ~_BV(PC3);
  } else if (1 == which) {
    PORTC &= ~_BV(PC2);
    PORTC |= _BV(PC3);
  } else if (2 == which) {
    PORTC |= _BV(PC2);
    PORTC |= _BV(PC3);
  }
}

static void slog(const char * prefix, int8_t index, int16_t val)
{
  syslog_putc(' ');
  syslog_putpgm(prefix);
  syslog_putc('0' + index);
  syslog_putc('=');
  syslog_i16d(val);
}

#define SLOG(lit, i, v) \
{ static const char s[] PROGMEM = lit; slog(s, i, v); }

uint8_t completed = 0;
uint16_t duration[3];

int8_t got_fix(int8_t which)
{
  int8_t n;

  gps_select(which);
  tx_msg("## Selected ", which);
  ydelay(50);

  cbuf_initialise(spt_rx);            /* XXX flush buffer */

  n = ub_read_nrsats();
  SLOG("nr_sats_", which, n);
  if (n < 0) {
    n = ub_read_nrsats();
    SLOG("nr_sats_", which, n);
  }
  n = ub_read_position();
  SLOG("position_", which, n);
  if (n) {
    n = ub_read_position();
    SLOG("position_", which, n);
  }

  return(n?0:1);
}

void fg_task(void)
{
  uint32_t start_time;
  uint16_t elapsed;

  /*
    Not calling ub_start(), rolling our own.
   */
  DDRC |= _BV(PC2);
  DDRC |= _BV(PC3);

  gps_power_on();
  gps_select(0);
  spt_set_speed_9600();
  spt_rx_start();
  tx_puts("## Initialised\r\n");

  for ( ; ; ) {
    int i;
    uint32_t timeout;
    gps_power_on();
    syslog_attr("gps_power", 1);
    ydelay(100);
    completed = 0;
    start_time = time_uptime;
    timeout = time_uptime + 300;

    while (completed != 0x7) {
      for (i = 0; i < 3; i++) {
        if (completed & (1<<i)) continue;
        if (got_fix(i)) {
          completed |= (1<<i);
          elapsed = time_uptime - start_time;
          SLOG("duration_", i, elapsed);
        } else if (time_uptime >= timeout) {
          completed |= (1<<i);
          elapsed = 999;
          SLOG("duration_", i, elapsed);
        }
        yield();
      }
    }
    gps_power_off();
    syslog_attr("gps_power", 0);
    /*
      Sleep a long time.
     */
    for (i = 0; i < 240; i++)
      ydelay(100);
  }
}
