/* (C) 2019 Harold Tay GPLv3 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <stdint.h>
#include "time.h"
#include "tx.h"
#include "nav.h"

#define TEST_PROFILE(arg) \
sleep_mode(); \
tx_puts(#arg); \
start = TCNT1; \
loc = arg; \
end = TCNT1; \
tx_puts(" = "); tx_puthex32(loc); tx_msg(", ", end - start);

static uint32_t delay(void)
{
  _delay_us(100);
  return(0UL);
}

void putpoint(struct nav_pt * p)
{
  tx_puts("(lon=");
  tx_putdec32(p->lon);
  tx_puts(",lat=");
  tx_putdec32(p->lat);
  tx_putc(')');
}

struct nav_pt fr, to;
void do_rhumb(void)
{
  uint16_t start, end;
  struct nav nav;

  start = TCNT1;
  nav = nav_rhumb(&fr, &to);
  end = TCNT1;
  if (start > end) end += TIMER_TOP + 1;
  tx_puts("From: "); putpoint(&fr);
  tx_puts(" To: "); putpoint(&to);
  tx_puts(", "); tx_putdec(end - start);
  tx_puts("us, hdg ");
  tx_putdec(nav.heading);
  tx_msg(", range ", nav.range);
}
void test_rhumb(uint32_t fr_lon, uint32_t fr_lat, int16_t d_lon, int16_t d_lat)
{
  fr.lon = fr_lon;
  fr.lat = fr_lat;
  to.lon = fr_lon + d_lon;
  to.lat = fr_lat + d_lat;
  do_rhumb();
}

void test_rhumb_8_square(uint32_t fr_lon, uint32_t fr_lat, int16_t disp)
{
  test_rhumb(fr_lon, fr_lat,  0,     disp);
  test_rhumb(fr_lon, fr_lat,  disp,  disp);
  test_rhumb(fr_lon, fr_lat,  disp,  0);
  test_rhumb(fr_lon, fr_lat,  disp, -disp);
  test_rhumb(fr_lon, fr_lat,     0, -disp);
  test_rhumb(fr_lon, fr_lat, -disp, -disp);
  test_rhumb(fr_lon, fr_lat, -disp,     0);
  test_rhumb(fr_lon, fr_lat, -disp,  disp);
}

void test_reckon(uint32_t fr_lon, uint32_t fr_lat, int16_t dx, int16_t dy)
{
  uint16_t start, end;

  to.lon = fr.lon = fr_lon;
  to.lat = fr.lat = fr_lat;

  start = TCNT1;
  if (nav_reckon(&to, dx, dy))
    tx_puts("nav_reckon() returned ERROR\r\n");
  end = TCNT1;
  if (start > end) end += TIMER_TOP + 1;

  tx_putdec(end - start);
  tx_puts(" us to reckon "); putpoint(&fr); tx_puts(" + ");
  tx_putdec(dx); tx_puts(" m E + ");
  tx_putdec(dy); tx_puts(" m N = ");
  putpoint(&to);
}

int
main(void)
{
  uint16_t start, end;
  nav_t loc;

  tx_init();
  sei();
  time_init();

  tx_puts("Starting\r\n");

  TEST_PROFILE(delay())
  TEST_PROFILE(nav_make_nav_t(216000000L-1))
  TEST_PROFILE(nav_make_nav_t(216000000L))
  TEST_PROFILE(nav_make_nav_t(216000000L/2))
  TEST_PROFILE(nav_make_nav_t(-216000000L/2))

#define LON_0 0
#define LON_90 ((1UL<<26)/4)
#define LON_180 ((1UL<<26)/2)
#define LON_270 (3*LON_90)

#define LAT_0 0
#define LAT_45P (1UL<<26)/8
#define LAT_45N ((1UL<<26) - LAT_45P)
#define LAT_67P (((1UL<<26)/4) - ((1UL<<26)/16))
#define LAT_67N ((1UL<<26) - LAT_67P)

#define RHUMB_TEST(lon, lat, disp) \
tx_puts("\r\n" #lon ", " #lat "\r\n"); \
test_rhumb_8_square(lon, lat, disp);

  RHUMB_TEST(LON_0, LAT_0, 100)

  RHUMB_TEST(LON_180, LAT_0, 100)

  RHUMB_TEST(LON_0, LAT_45P, 100)

  RHUMB_TEST(LON_90, LAT_45P, 100)

  RHUMB_TEST(LON_0, LAT_45N, 100)

  RHUMB_TEST(LON_90, LAT_45N, 100)

  RHUMB_TEST(LON_0, LAT_67P, 100)

  RHUMB_TEST(LON_0, LAT_67N, 100)

  RHUMB_TEST(LON_270, LAT_67P, 100)

  RHUMB_TEST(LON_270, LAT_67N, 100)

  /* 12A car park to roundabout */
  tx_puts("12A cp to roundabout:\r\n");
  fr.lon = 19345068UL; fr.lat = 241831UL;
  to.lon = 19345165UL; to.lat = 241728UL;
  do_rhumb();
  tx_puts("\r\n");

#if 0  /* nav_reckon() has been removed */
#define RECKON_TEST(lon, lat, x, y) \
tx_puts("\r\n" #lon ", " #lat "\r\n"); \
test_reckon(lon, lat, x, y);

  RECKON_TEST(LON_0, LAT_0, 100, 100)
  RECKON_TEST(LON_180, LAT_0, 100, 100)
  RECKON_TEST(LON_0, LAT_45P, 100, 100)
  RECKON_TEST(LON_90, LAT_45P, 100, 100)
  RECKON_TEST(LON_0, LAT_45N, 100, 100)
  RECKON_TEST(LON_0, LAT_67P, 100, 100)
  RECKON_TEST(LON_0, LAT_67N, 100, 100)
  RECKON_TEST(LON_270, LAT_67P, 100, 100)
  RECKON_TEST(LON_270, LAT_67N, 100, 100)
#endif
  for( ; ; );
}
