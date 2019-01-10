/* (C) 2019 Harold Tay GPLv3 */

/*
  Test nav code based on know-good inputs/outputs.
From http://edwilliams.org/gccalc.htm
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <stdint.h>
#include "time.h"
#include "tx.h"
#include "nav.h"
#include <avr/pgmspace.h>

const struct leg {
  struct nav_pt src, dst;
  int8_t heading;
  int32_t range;
  char desc[40];
} legs[] PROGMEM = {
/* #define P(d) (d<0?360.0-d:d) */
#define P(d) d
#define AM(deg)   (nav_t)(11930464.7111111111*(P(deg)))
#define SLIV(deg) (uint8_t)(256.0*(deg/360.0))
#define AMM(km)    (1000L*(km/0.596))
#define DATA(slat,slon,dlat,dlon,hdg,km) \
  {{ AM(slat), AM(slon) }, { AM(dlat), AM(dlon) }, SLIV(hdg), AMM(km),\
  #slat "," #slon "," #dlat "," #dlon "," #hdg "," #km },
#define OKAY(a,b,c,d,e,f) /* nothing */
  DATA(0.1,0,      -0.1,0,      180,22.1)
  DATA(0.1,179.0,  -0.1,179.0,  180,22.1)
  DATA(0.1,179.9,  -0.1,-179.9, 134.8,31.4)
  DATA(89.0,179.9,  89.0,-179.9,89.9,0.39)
  DATA(89.0,0.1,    89.0,-0.1,  270.1,0.39)
  DATA(-89.0,0.1,  -89.0,-0.1,  270.0,0.39)
  DATA(-89.0,0.1,  -89.2,-0.1,  180.8,22.34)
  DATA(-89.0,0.2,  -89.2,-0.2,  181.6,22.35)
  DATA(50.0,10.0,   50.1,10.0,  0.0,11.123)
  DATA(-50.0,10.0, -50.1,10.0,  180.0,11.123)
  DATA(-50.0,-10.0,-50.1,-10.0, 180.0,11.123)
  DATA(50.0,-10.0,  50.1,-10.0, 0.0,11.123)
  DATA(50.0,-12.9,  50.1,-13.0, 327.3,13.23)
  DATA(50.0,-120.9, 50.1,-121.0,327.26,13.23)
  DATA(70.0,-120.9, 70.1,-121.0,341.19,11.79)
  DATA(70.0,120.9,  70.1,121.0, 18.81,11.79)
  DATA(-80.0,120.9,-80.1,121.0, 170.244,11.33)
  DATA(-89.0,120.9,-89.1,121.0, 179.1,11.17)
  DATA(-45.0,120.9,-44.9,121.0, 35.41,13.63)
  DATA(-45.0,120.9,-44.9,121.1, 54.92,19.30)
  DATA(-45.0,45.0, -44.9,45.0,  0.0,11.11)
  DATA(-45.0,45.0, -44.9,45.1,  35.41,13.63)
  DATA(60.0,45.0,   60.2,45.1,  13.97,22.97)
  DATA(0.1,60.0,   -0.2,60.1,   161.45,34.99)
  DATA(0.1,0.2,    -0.2,-0.3,   239.21,64.80)
};

static void check_conversion(nav_t n)
{
  int32_t dmm;

  tx_putdec32(n); tx_puts("(nav_t) =? ");
  dmm = nav_make_dmm(n);
  tx_putdec32(dmm); tx_puts("(dmm) =? ");
  n = nav_make_nav_t(dmm);
  tx_putdec32(n); tx_puts("(nav_t)\r\n");
}

int
main(void)
{
  struct leg lg;
  struct nav nav;
  int8_t i, j;

  tx_init();
  for (i = 2; i >= 0; i--) {
    tx_puts("\r\nDelay "); tx_putc('0' + i);
    _delay_ms(900);
  }
  tx_puts("\r\n");

  for (i = 0; i < sizeof(legs)/sizeof(*legs); i++) {
    unsigned char * src, * dst;
    dst = (void *)&lg;
    src = (void *)(legs + i);
    for (j = 0; j < sizeof(struct leg); j++) {
      dst[j] = pgm_read_byte_near(src + j);
    }

    nav = nav_rhumb(&lg.src, &lg.dst);
    tx_putdec(i); tx_putc(' ');
    tx_puts(lg.desc); tx_puts(":\r\n ");
    tx_puts("src:");
    tx_putdec32(lg.src.lat); tx_putc(',');
    tx_putdec32(lg.src.lon);
    tx_puts(" dst:");
    tx_putdec32(lg.dst.lat); tx_putc(',');
    tx_putdec32(lg.dst.lon);
    tx_puts(" heading:");
    tx_putdec(lg.heading); tx_puts(" ?= "); tx_putdec(nav.heading);
    tx_puts(" range:");
    tx_putdec32(lg.range); tx_puts(" ?= "); tx_putdec32(nav.range);
    tx_puts("\r\n");
  }

  for (i = 0; i < sizeof(legs)/sizeof(*legs); i++) {
    check_conversion(legs[i].src.lat);
    check_conversion(legs[i].src.lon);
    check_conversion(legs[i].dst.lat);
    check_conversion(legs[i].dst.lon);
  }

  for ( ; ; );
}

