/* (C) 2019 Harold Tay LGPLv3 */
#include <avr/pgmspace.h>
#include "cal.h"

#undef DEBUG
#ifdef DEBUG
#include "tx.h"
#define DBG(x) x
#else
#define DBG(x) /* nothing */
#endif

/* All dates/times in UTC.  Epoch starts on 2017.  */

#define LEAPDAYS(year) !!((0 == (year % 4)) && (year != 100))

uint32_t cal_seconds(uint8_t dmy[3], uint8_t hms[3])
{
  uint8_t y;
  uint8_t i;
  uint16_t days;
  uint32_t seconds;
  static const uint8_t daysinmonth[] PROGMEM =
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

  if (dmy[2] > 99) return(0);
  if (dmy[2] < 17) return(0);
  if (dmy[1] > 12 || dmy[1] < 1) return(0);
  y = dmy[2];
  if (0 == y) y = 100; /* year 2100 */
  if (dmy[0] > pgm_read_byte_near(daysinmonth+dmy[1]-1) + LEAPDAYS(y))
    return(0);

  if (hms[0] > 23) return(0);
  if (hms[1] > 59) return(0);
  if (hms[2] > 60) return(0);

  /*
    We only deal with years 2017 to 2100, so:
    Leap years: 2020, 2024, 2032, ... but not 2100
   */

  days = 0;
  for (i = 17; i < y; i++)
    days += 365 + LEAPDAYS(i);

  for (i = 0; i < dmy[1]-1; i++)
    days += pgm_read_byte_near(daysinmonth+i);
  if (dmy[1] > 2) days += LEAPDAYS(y);
  days += (dmy[0]-1);                 /* counting starts at 1 */

  DBG(tx_puts("Days = "));
  DBG(tx_putdec(days));

  seconds = days * 24 + hms[0];       /* hours */
  seconds *= 60;
  seconds += hms[1];                  /* minutes */
  seconds *= 60;
  seconds += hms[2];                  /* seconds */
  return(seconds);
}

#ifdef DEBUG
#include "tx.h"
#include "yield.h"

void cal_task(void)
{
  uint32_t t;
  uint8_t dmy[3] = { 19, 9, 17 };
  uint8_t hms[3] = { 5, 54, 47 };
  t = cal_seconds(dmy, hms);
  tx_puts("Convert 19/9/17 5:54:47 = ");
  tx_putdec32(t);
  tx_puts("\r\n");
  for ( ; ; ) yield();
}
#endif
