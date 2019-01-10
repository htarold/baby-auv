/* (C) 2019 Harold Tay LGPLv3 */
/*
  From http://www.romanblack.com/integer_degree.htm
 */
#include <avr/pgmspace.h>
#include <stdlib.h>                   /* for div() */
#include "imaths.h"

/*
  Compute atan2 where y < x i.e. answer is < pi/4
  y and x are positive.
 */
static uint8_t canonical(int16_t y, int16_t x)
{
  div_t r;
  uint8_t flip, angle;

  static const uint8_t PROGMEM tantab[65] = {
     0,  1,  1,  2,  3,  3,  4,  4,  5,  6,  6,  7,  8,  8,  9,  9,
    10, 11, 11, 12, 12, 13, 13, 14, 15, 15, 16, 16, 17, 17, 18, 18,
    19, 19, 20, 20, 21, 21, 22, 22, 23, 23, 24, 24, 25, 25, 25, 26,
    26, 27, 27, 27, 28, 28, 29, 29, 29, 30, 30, 30, 31, 31, 31, 32, 32,
  };

  flip = (y > x);
  if (flip) {
    int16_t tmp;
    tmp = y;
    y = x;
    x = tmp;
  }

  if (0 == x) return(0);

  while (x>=512) {
    x /= 2;
    y /= 2;
  }
  /*
    Now denominator is < 1024 and numerator < denominator
   */

  r = div(y*64, x);
  if (r.rem >= x/2) r.quot++;
  /*
    r.quot <= 64
   */
  angle = pgm_read_byte(tantab + r.quot);
  return(flip?64-angle:angle);
}

/* 43us */
int16_t iatan2(int16_t y, int16_t x)
{
  uint8_t a;

  /* which quadrant? */
  if (y > 0) {
    if (x > 0) {  /* Q1 */
      a = canonical(y, x);
    } else {      /* Q2 */
      a = 128 - canonical(y, 0 - x);
    }
  } else {
    if (x > 0) {  /* Q4 */
      a = 256 - canonical(0 - y, x);
    } else {      /* Q3 */
      a = 128 + canonical(0 - y, 0 - x);
    }
  }
  return(a);
}

/*
  Integer sqrt:
  http://www.embedded.com/electronics-blogs/
  programmer-s-toolbox/4219659/Integer-Square-Roots
  520us.
 */

uint16_t isqrt(uint32_t a)
{
  uint32_t rem;
  uint32_t root;
  int8_t i;

  rem = root = 0UL;
  for(i = 0; i < 16; i++){
    root <<= 1;
    rem = ((rem << 2) + (a >> 30));
    a <<= 2;
    root++;
    if( root <= rem ){
      rem -= root;
      root++;
    }else
      root--;
  }
  return((uint16_t)(root >> 1));
}

/*
  Only do for pi/2 (90 degrees), 64 entries.
  Have to be able to generate +1 and -1, which leads to 2 lost
  bits.
 */

static const int16_t PROGMEM sintab[65] = {
    0,   25,   50,   75,  100,  125,  150,  175,
  200,  224,  249,  273,  297,  321,  345,  369,
  392,  415,  438,  460,  483,  505,  526,  548,
  569,  590,  610,  630,  650,  669,  688,  706,
  724,  742,  759,  775,  792,  807,  822,  837,
  851,  865,  878,  891,  903,  915,  926,  936,
  946,  955,  964,  972,  980,  987,  993,  999,
  1004, 1009, 1013, 1016, 1019, 1021, 1023, 1024, 1024
};

int16_t isin1024(uint8_t slivs)
{
  if (slivs <= 64)
    return(pgm_read_word(sintab + slivs));
  if (slivs <= 128)
    return(pgm_read_word(sintab + 128 - slivs));
  if (slivs <= 192)
    return(0 - pgm_read_word(sintab + slivs - 128));
  return(0 - pgm_read_word(sintab + 256 - slivs));
  /* 3 us */
}

int16_t icos1024(uint8_t slivs)
{
  return(isin1024(slivs + 64));
}

int8_t iasin(int16_t s)
{
  uint8_t neg, hi, lo, mid;
  int16_t arc;

  neg = (s < 0);
  if (neg) s = 0 - s;

  /*
    Binary search.
   */
  hi = 64;
  lo = 0;
  for( ; ; ){
    mid = (hi + lo)/2;
    arc = pgm_read_word(sintab + mid);
    if (arc == s) break;
    if (arc > s) hi = mid;
    else lo = mid;
    if (lo + 1 >= hi) break;
  }
  /*
    32us
   */
  return(neg?-mid:mid);
}
