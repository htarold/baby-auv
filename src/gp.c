/* (C) 2019 Harold Tay LGPLv3 */
/*
  Read NMEA fields.
>0117.82951,N,10346.50263,E,1,08,<
>0117.82963,N,10346.50233,E,1,09,<
>0117.82914,N,10346.50194,E,1,08,<
>0117.82927,N,10346.50190,E,1,08,<
>0117.82948,N,10346.50187,E,1,08,<
>0117.82982,N,10346.50198,E,1,10,<
>0117.83016,N,10346.50209,E,1,10,<
 */

#include <stdint.h>
#include "gp.h"

#undef DEBUG
#ifdef DEBUG
#include "tx.h"
#define DBG(x) x
#else
#define DBG(x) /* nothing */
#endif

static int32_t accum;
static uint8_t dec_convert(char * p, uint8_t nchars)
{
  uint8_t i;
  uint16_t res;

  res = 0;
  for (i = 0; i < nchars; i++) {
    if (p[i] < '0' || p[i] > '9') break;
    res *= 10;
    res += p[i] - '0';
  }
  accum += res;
  return(i);
}

static uint8_t conv_minutes(char * p)
{
  char * start;
  uint8_t i;

  start = p;
  accum *= 60;                        /* convert to minutes */
  i = dec_convert(p, 2);              /* add minutes */
  if (i != 2) return(0);
  DBG(tx_puts("# 1 ok\r\n"));
  accum *= 10000;                     /* 0.1 milliminutes */
  p += 2;
  if (*p != '.') return(0);
  DBG(tx_puts("# 2 ok\r\n"));
  p++;
  i = dec_convert(p, 4);
  if (i < 4) return(0);
  DBG(tx_puts("# 3 ok\r\n"));
  for (p += i; *p && *p != ','; p++);
  if (*p != ',') return(0);
  DBG(tx_puts("# 4 ok\r\n"));
  p++;
  if ('N' == *p || 'E' == *p) ;
  else if ('S' == *p || 'W' == *p) accum = 0 - accum;
  else return(0);
  DBG(tx_puts("# 5 ok\r\n"));
  p++;
  if (',' != *p) return(0);
  DBG(tx_puts("# 6 ok\r\n"));
  p++;
  return(p - start);
}

uint8_t gps_lat(char * p, int32_t * result)
{
  uint8_t i;
  char * start;

  start = p;
  accum = 0;
  i = dec_convert(p, 2);              /* whole degrees */
  if (i != 2) return(0);
  p += 2;
  i = conv_minutes(p);
  if (!i) return(0);
  p += i;
  *result = accum;
  DBG(tx_puts("Returning:"));
  DBG(tx_puts(p));
  DBG(tx_puts("\r\n"));
  return(p - start);
}

uint8_t gps_lon(char * p, int32_t * result)
{
  uint8_t i;
  char * start;

  start = p;
  accum = 0;
  i = dec_convert(p, 3);              /* whole degrees */
  DBG(tx_puts(p));
  DBG(tx_puts("\r\n"));
  if (i != 3) return(0);
  p += 3;
  i = conv_minutes(p);
  if (!i) return(0);
  p += i;
  *result = accum;
  return(p - start);
}

uint8_t gps_date(char * p, uint8_t dmy[3])
{
  uint8_t len, i;

  for (i = 0; i < 3; i++, p += 2) {
    accum = 0;
    len = dec_convert(p, 2);
    if (len != 2) return(0);
    dmy[i] = accum;
  }
  return(7);
}

uint8_t gps_time(char * p, uint8_t hmsh[4])
{
  if (7 != gps_date(p, hmsh))
    return(0);
  if ('.' == p[6]) {
    accum = 0;
    if (2 == dec_convert(p+7, 2)) {
      hmsh[3] = accum;
      return(10);
    }
  }
  hmsh[3] = 0;
  return(7);
}
