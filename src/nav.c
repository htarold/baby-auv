/* (C) 2019 Harold Tay LGPLv3 */
/*
  Chart plotting routines.
 */

#include <avr/pgmspace.h>
#include <stdint.h>
#include "imaths.h"
#include "nav.h"

#undef DEBUG
#ifdef DEBUG
#include "tx.h"
#define DBG(x) x
#else
#define DBG(x) /* nothing */
#endif

/*
  This converts a lat or a lon in minutes/10,000 to local angular
  measure.
  There are 2^26 measures in a circle.  These are LEFT justified
  to 32 bits, so that arithmetic overflows in a natural way.

  Since circumference of earth is 40.075e6m, 1 local unit
  corresponds to 40.075e6/2^26 = 0.597164.....m
  or 0x0.98DFBDFF metres, and 1 metre corresponds to
  0x1.ACB163E23FA2C... angular measures.

  GPGGA provides the minutes to 4 decimal places, i.e. full
  scale is 216 million (about 29 bits).  Scale to 2^26 or 67108864.
  gps/216e6 = am/2^26
  angular measure = gps*2^26/216e6
  In binary:
  angular measure = gps*0.01001111100010010101001110010001
  (keeping 32 bits)
 */

/*
  nav_t = ((1<<26)*dmm/(360*60*10000))
  simplifying:
  nav_t = ((1<<17)*dmm/421875)
  nav_t = ((1<<17)*dmm/(27*15625))
  nav_t = dmm*0x4f895392
  THEN:
  nav_t <<= 6
 */
nav_t nav_make_nav_t(int32_t gps)
{
  int64_t tmp;
  nav_t n;
  /* if (gps < 0) gps = 0 - gps; */
#define NUMERATOR 0x4F895392LL
#define DENOMINATOR (1L<<26)
  tmp = gps * NUMERATOR;
  n = tmp / DENOMINATOR;
  return(n);
}

int32_t nav_make_dmm(nav_t n)
{
  int64_t tmp;
  /*
    mmin = n * 21600UL*1000*10 / 2^26
   */
  tmp = n * 216000000LL;
  tmp /= (1LL<<32);
  return((int32_t)tmp);
}

static int16_t cos_lat;

int16_t nav_latitude_correction(void) { return(cos_lat); }

/*
  Given 2 waypoints, calculate range (angular) and heading.
  XXX MAY NOT WORK AT POLES.
 */

struct nav nav_rhumb(struct nav_pt * fr, struct nav_pt * to)
{
  /*
    Construct a rectangle dlon tall, and dlat * cos(lat) wide.
    Angle of the diagonal is the heading, its length is the range.
   */
  int32_t dlon, dlat;
  struct nav nav;
  uint8_t nr_shifts;

  dlat = to->lat - fr->lat;
  dlon = to->lon - fr->lon;
  DBG(tx_puts("##dlat:")); DBG(tx_putdec32(dlat));
  DBG(tx_puts(" #to->lon =")); DBG(tx_putdec32(to->lon));
  DBG(tx_puts(" #fr->lon =")); DBG(tx_putdec32(fr->lon));
  DBG(tx_puts(" #dlon:")); DBG(tx_putdec32(dlon));
  DBG(tx_puts("\r\n"));
  /*
    we need just the 8 highest bits (2^8) to pass
    to icos1024().
   */
  cos_lat = icos1024((uint8_t)(fr->lat >> 24));
  if (cos_lat < 0) cos_lat = 0 - cos_lat; /* shouldn't be needed */
  DBG(tx_puts(" #cos_lat:")); DBG(tx_putdec(cos_lat));

  /*
    Scale down the trig to avoid overflow later.
   */
  dlon /= 64;
  dlat /= 64;

  DBG(tx_puts(" #dlat shifted:")); DBG(tx_putdec32(dlat));
  DBG(tx_puts(" #dlon shifted:")); DBG(tx_putdec32(dlon));
  /*
    Scale the horizontal measure by the cosine of the latitude.
   */
  dlon = ((dlon/8) * (cos_lat/2))/64;
  DBG(tx_puts(" #dlon adjusted:")); DBG(tx_putdec32(dlon));
  DBG(tx_puts("\r\n"));

  for(nr_shifts = 0; ; nr_shifts++) {
    if (dlon > INT16_MAX
     || dlon < INT16_MIN
     || dlat > INT16_MAX
     || dlat < INT16_MIN) {
      dlon /= 2;
      dlat /= 2;
    } else
      break;
  }

  nav.range = isqrt(dlat*dlat + dlon*dlon);
  for ( ; nr_shifts; nr_shifts--)
    nav.range *= 2;
  DBG(tx_puts(" #nav.range="));
  DBG(tx_putdec32(nav.range));
  DBG(tx_puts("\r\n"));

  nav.heading = iatan2(dlon, dlat);
  return(nav);
}

#if 0  /* May not be using this buggy function */
/*
  We've travelled north metres north and east metres east.
  Where are we?
 */
int8_t nav_reckon(struct nav_pt * position, int16_t north, int16_t east)
{
  int32_t dx, dy;
  int16_t c;

#define LAT_90P ((1UL<<26)/4)
#define LAT_90N ((1UL<<26)-LAT_90P)

  dy = north + multiply_scaled(north, 0xACB163E2);
  dx = east + multiply_scaled(east, 0xACB163E2);
  c = icos1024(position->lat>>18);
  if (!c) return(-1);
  dx = (dx * 1024)/c;
  position->lon += dx;
  position->lat += dy;
  /* Overshot a pole? */
  if (position->lat > LAT_90P && position->lat < LAT_90N) {
    position->lat = (1UL<<26)/2 - position->lat;
    position->lat &= (1UL<<26)-1;
    /* After latitude is normalised, longitude is flipped */
    position->lon += (1UL<<26)/2;
    position->lon &= (1UL<<26)-1;
  }
  return(0);
}
#endif
