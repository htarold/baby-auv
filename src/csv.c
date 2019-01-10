/* (C) 2019 Harold Tay LGPLv3 */
/*
  Parsing NMEA fields.
 */
#include <stdint.h>
#include <string.h>
#include "spt.h"                      /* front bulkhead comms */
#include "nav.h"
#include "csv.h"

#undef DEBUG
#ifdef DEBUG
#include "tx.h"
#include "syslog.h"
#define DBG(x) x
#else
#define DBG(x) /* nothing */
#endif

/*
  csv_*() return number of input chars consumed.
 */

int32_t csv_accum;
uint8_t csv_numeric(char * p, uint8_t nchars)
{
  uint8_t i;
  uint16_t res;

  res = 0;
  for (i = 0; i < nchars; i++) {
    if (p[i] < '0' || p[i] > '9') break;
    res *= 10;
    res += p[i] - '0';
  }
  csv_accum += res;
  return(i);
}

uint8_t csv_minutes(char * p)
{
  char * start;
  uint8_t i;

  start = p;
  csv_accum *= 60;                    /* convert to minutes */
  i = csv_numeric(p, 2);              /* add minutes */
  if (i != 2) return(0);
  csv_accum *= 10000;                 /* 0.1 milliminutes */
  p += 2;
  if (*p != '.') return(0);
  p++;
  i = csv_numeric(p, 4);
  if (i < 4) return(0);               /* too short */
  for (p += i; *p && *p != ','; p++);
  if (*p != ',') return(0);
  p++;
  if ('N' == *p || 'E' == *p) {
    ;
  } else if ('S' == *p || 'W' == *p) {
    DBG(tx_puts("## lat or lon is negative:"));
    DBG(tx_putdec32(csv_accum));
    DBG(tx_puts("<->"));
    csv_accum = 0UL - csv_accum;
    DBG(tx_putdec32(csv_accum));
    DBG(tx_puts("\r\n"));
  } else
    return(0);
  p++;
  if (',' != *p) return(0);
  p++;
  return(p - start);
}

/*
  Assumes 2 digits of degrees.
  Accum will be in unites of minutes/10,000.
 */
uint8_t csv_degmins(char * p)
{
  uint8_t i;
  char * start;

  DBG(tx_puts("## csv_degmins: "));
  DBG(tx_puts(p));
  DBG(tx_puts("\r\n"));
  start = p;
  csv_accum = 0;
  i = csv_numeric(p, 2);              /* whole degrees */
  if (i != 2) return(0);
  p += 2;
  i = csv_minutes(p);
  if (!i) return(0);
  p += i;
  DBG(tx_puts("## csv_degmins: "));
  DBG(tx_putdec32(csv_accum));
  DBG(tx_puts("\r\n"));
  return(p - start);
}

uint8_t csv_dateortime(char * p, uint8_t dmy[3])
{
  uint8_t len, i;

  for (i = 0; i < 3; i++, p += 2) {
    csv_accum = 0;
    len = csv_numeric(p, 2);
    if (len != 2) return(0);
    dmy[i] = csv_accum;
  }
  return(6);
}

#define FACTOR (67108864.0/216000000.0)
uint8_t csv_latlon(char * p, struct nav_pt * np)
{
  int8_t c;
  char * s, x_100_degs;
  int32_t tmp;

  s = p;
  c = csv_degmins(p);
  if (!c) return(0);
  p += c;
  np->lat = nav_make_nav_t(csv_accum);
  DBG(syslog_lattr("lat_a", csv_accum));
  DBG(syslog_lattr("lat_b", np->lat));

  /* longitude contains extra digit for 100's of degrees; kept in ch */
  x_100_degs = *p;
  if (x_100_degs > '3' || x_100_degs < '0') return(0);
  p++;
  c = csv_degmins(p);
  if (!c) return(0);
  p += c;
  tmp = (x_100_degs - '0') * 100 * 60 * 10000L;
  if (csv_accum < 0) tmp = 0 - tmp;
  csv_accum += tmp;
  np->lon = nav_make_nav_t(csv_accum);
  DBG(syslog_lattr("lon_a", csv_accum));
  DBG(syslog_lattr("lon_b", np->lon));

  return(p - s);
}
