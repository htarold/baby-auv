/* (C) 2019 Harold Tay GPLv3 */
/*
  Test harness for odo.
 */

#include "prng.h"
#include "tx.h"
#include "nav.h" /* for structure defs */
#include "tude.h"
#include "ee.h"
#include "syslog.h"
#include "odo.h"
#include "imaths.h"
#include "bg.h"
#include <util/delay.h>

static struct {
  uint32_t now;
  uint16_t revs;
  uint8_t heading;
  uint8_t rpm;
} context;

int8_t ub_datetime(uint32_t t)
{
  t = context.now;
  return(0);
}

int8_t thrust_get_percent(void) { return(80); }

int8_t tude_get(struct angles * ap, uint8_t unused)
{
  ap->heading = context.heading;
  return(0);
}

uint16_t thrust_get_revs(void)
{
  return(context.revs);
}

struct ee ee;
uint8_t ee_load(void * a0, void * a1, uint8_t unused)
{
  struct odo * op;
  op = a0;
  op->calib1024 = 309;
  return(0);
}
void ee_store(void * a0, void * a1, uint8_t unused) { return; }

void syslog_attrpgm(const char * name, int16_t val)
{
  tx_putpgms(name);
  tx_putc('=');
  tx_putdec(val);
  tx_puts("\r\n");
}

#define PERIOD 10 /* seconds between calls to odo_periodically() */

/*
  Assume 120 RPM or 2 revs/second, 0.309 angular measures per
  second.
 */
void go(uint8_t hdg, uint16_t seconds)
{
  extern struct cartesian auv;
  uint16_t s;

  /*
    Convert to distance per PERIOD
   */

  for (s = 0; s < seconds; s += PERIOD) {
    struct { int rnd:3; } rnd;
    odo_periodically();
    context.now += PERIOD;
    rnd.rnd = prng();
    context.heading = hdg + rnd.rnd;
    context.revs += (context.rpm * PERIOD) / 60;
#if 0
    tx_puts("seconds, heading, revs, north, east = ");
    tx_putdec(s); tx_putc(',');
    tx_putdec(context.heading); tx_putc(',');
    tx_putdec(context.revs); tx_putc(',');
    tx_putdec(auv.north); tx_putc(',');
    tx_putdec(auv.east); tx_puts("\r\n");
#endif
  }
}

static void print_latlon(char * msg, struct nav_pt * p)
{
  tx_puts(msg);
  tx_puts(": lat=");
  tx_putdec32(p->lat);
  tx_puts(" lon=");
  tx_putdec32(p->lon);
  tx_puts("\r\n");
}
static void print_rhumb(struct nav * np)
{
  tx_puts("heading:"); tx_putdec(np->heading);
  tx_puts(" range:"); tx_putdec32(np->range);
  tx_puts("\r\n");
}

int
main(void)
{
  struct nav_pt src, dst;
  struct cartesian drift;
  struct nav n;
  int8_t i;

  tx_init();
  for (i = 2; i >=0; i--) {
    tx_puts("\r\nDelay ");
    tx_putc('0' + i);
    _delay_ms(900);
  }

  odo_init();
  context.rpm = 120; /* constant */
  /*
    Make a square: north 1000, east 1000, south 1000, west 1000.
   */
  src.lat = src.lon = 0;

  print_latlon("Origin", &src);
  odo_start(&src);
  go(0, 1000/* seconds */);
  drift.north = drift.east = 0;
  odo_position(&dst, &drift);
  print_latlon("North", &dst);
  n = nav_rhumb(&src, &dst);
  print_rhumb(&n);
  odo_stop(&dst, &drift);

  src = dst;
  odo_start(&src);
  go(64, 1000);
  drift.north = drift.east = 0;
  odo_position(&dst, &drift);
  print_latlon("East", &dst);
  n = nav_rhumb(&src, &dst);
  print_rhumb(&n);
  odo_stop(&dst, &drift);

  src = dst;
  odo_start(&src);
  go(128, 1000);
  drift.north = drift.east = 0;
  odo_position(&dst, &drift);
  print_latlon("South", &dst);
  n = nav_rhumb(&src, &dst);
  print_rhumb(&n);
  odo_stop(&dst, &drift);

  src = dst;
  odo_start(&src);
  go(192, 1000);
  drift.north = drift.east = 0;
  odo_position(&dst, &drift);
  print_latlon("West", &dst);
  n = nav_rhumb(&src, &dst);
  print_rhumb(&n);
  odo_stop(&dst, &drift);

  for ( ; ; ) ;
}
