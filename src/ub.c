/*
  (C) 2019 Harold Tay LGPLv3
  Ublox gps unit (applies to NMEA in general).

 */
#include <stdint.h>
#include <string.h>
#include <avr/interrupt.h>            /* only for sei() */
#include "spt.h"                      /* front bulkhead comms */
#include "cbuf.h"
#include "ub.h"
#include "yield.h"
#include "nav.h"
#include "sel.h"                      /* front bulkhead selector */
#include "cal.h"
#include "tx.h"
#include "time.h"
#include "fg.h"                       /* for fg_buffer[] */
#include "csv.h"

#define DEBUG
#ifdef DEBUG
#define DBG(x) x
#else
#define DBG(x) /* nothing */
#endif

/*
  Keep consing into scan_buffer.
  The separator after the token is also collected.
  On successful return, scan_buffer[here] points to \0 after the
  token.
 */

/* desired fields are copied into this: */
#define scan_buffer ((char *)fg_buffer)
#define SCAN_BUFFER_SIZE FG_BUFFER_SIZE

static uint8_t here, prev, checksum;
static int8_t scan(void)
{
  uint8_t retries;

  for (retries = 0; ; ) {
    char ch;
    if (cbuf_nochar(spt_rx)) {
      retries++;
      if (retries > 250) return(UB_ER_TMO);
      yield();
      continue;
    }
    retries = 0;
    if (here >= SCAN_BUFFER_SIZE) return(UB_ER_OVF);
    ch = cbuf_get(spt_rx);
    /* DBG(tx_putc(ch)); */
    scan_buffer[here++] = ch;
    checksum ^= ch;
    if (ch < '.') break;              /* field separator */
  }
  scan_buffer[here] = '\0';
  return(0);                          /* success */
}

static void accept(void) { prev = here; }
static void reject(void) { scan_buffer[here = prev] = '\0'; }

static uint8_t dehex(char * s)
{
  uint8_t i;
  uint8_t val = 0;
  for (i = 0; i < 2; i++, s++) {
    val <<= 4;
    if (*s <= '9') val += *s - '0';
    else if (*s <= 'F') val += 10 + (*s - 'A');
  }
  return(val);
}

/*
  For fields, values start at 002, and \002 refers to the
  field immediately after the name.
 */
static int8_t copy_fields(const char * sname, const char * fields)
{
  int8_t er;
  uint8_t retries, field_num, cs;

  here = prev = 0;                    /* reset for scan() */

  for (retries = 0; ; retries++) {
    uint8_t r, found;
    if (retries > 40) return(UB_ER_TMO);

    for (r = 0; ; r++) {
      uint8_t eor;
      if (r > 40) return(UB_ER_TMO);
      er = scan();
      if (er) return(er);
      eor = ('$' == scan_buffer[here-1]);
      reject();
      if (eor) break;
    }
    retries = 0;

    checksum = 0;

    er = scan();
    if (er) return(er);               /* get sentence name */
    found = ! strcmp_P(scan_buffer, sname);
    reject();
    if (found) break;
  }

  DBG(tx_puts("\tFound GPXXX\r\n"));

  field_num = 1;

  for (retries = 0; pgm_read_byte_near(fields); retries++) {
    if (retries > 20) return(UB_ER_TMO);
    er = scan();
    if (er) return(er);
    field_num++;
    if (pgm_read_byte_near(fields) == field_num) {
      accept();
      fields++;
    } else
      reject();
  }

  DBG(tx_puts("copy_fields got all fields\r\n"));

  for (retries = 0; ; retries++) {
    if (retries >= 20) return(UB_ER_TMO);
    er = scan();
    if (er) return(er);
    er = ('*' != scan_buffer[here-1]);
    reject();
    if (!er) break;
  }

  cs = (checksum ^= '*');

  er = scan();
  if (er) return(er);

  if (cs != dehex(scan_buffer+prev)) {
    er = UB_ER_CKSUM;
    DBG(tx_puts("Checksums don't match:calculated="));
    DBG(tx_puthex(cs));
    DBG(tx_puts("!="));
    DBG(tx_puts(scan_buffer+prev));
    DBG(tx_puts("\r\n"));
  } else {
    er = 0;
    DBG(tx_puts("copied fields: "));
    DBG(tx_puts(scan_buffer));
    DBG(tx_puts("\r\n"));
  }
  return(er);
}

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

int8_t ub_start(void)
{
  int8_t er;
  er = SEL_GPS_ON;
  if (er) return(UB_ER_SEL);
  spt_set_speed_9600();
  spt_rx_start();
  return(0);
}

void ub_stop(void)
{
  spt_rx_stop();
  SEL_GPS_OFF;
}

static struct nav_pt position;
static uint32_t position_useby = 0;

int8_t ub_read_nrsats(void)
{
  const static char gpgsv[] PROGMEM = "GPGSV,";
  const static char fields[] PROGMEM = "\004";
  int8_t er;

  spt_rx_start();
  er = copy_fields(gpgsv, fields);
  spt_rx_stop();
  if (er) return(er);
  yield();
  csv_accum = 0;
  if (!csv_numeric(scan_buffer, 2)) return(UB_ER_NODATA);
  return((uint8_t)csv_accum);
}

int8_t ub_read_position(void)
{
  int8_t er, c;
  char * p;

  const static char gpgga[] PROGMEM = "GPGGA,";
  const static char fields[] PROGMEM = "\003\004\005\006\007";

  DBG(tx_puts("ub_read_position calling copy_fields\r\n"));
  spt_rx_start();
  er = copy_fields(gpgga, fields);
  spt_rx_stop();
  DBG(tx_puts("ub_read_position back from copy_fields\r\n"));
  if (er) return(er);

  yield();
  p = scan_buffer;
  c = csv_latlon(p, &position);
  if (!c) return(UB_ER_PARSE);
  p += c;

  yield();

  csv_accum = 0;
  c = csv_numeric(p, 2);
  if (c && csv_accum >= 1 && csv_accum <= 7) {
    cli();
    position_useby = time_uptime + 90;
    sei();
    return(0);  /* got fix */
  }
  return(UB_ER_NOFIX);
}

int8_t ub_position(struct nav_pt * np)
{
  if (np) *np = position;
  return(time_uptime > position_useby?UB_ER_STALE:0);
}

/* test task */
void ub_echo(void)
{
  /* ub_start(); */
  for ( ; ; ) {
    if (cbuf_nochar(spt_rx)) {
      yield();
    } else
      tx_putc(cbuf_get(spt_rx));
  }
}

#ifdef DEBUG
static void print3(uint8_t ary[3])
{
  tx_putdec(ary[0]); tx_putc(' ');
  tx_putdec(ary[1]); tx_putc(' ');
  tx_putdec(ary[2]); tx_putc(' ');
}
#endif

static uint32_t epoch_offset;
static uint32_t epoch_offset_useby;

int8_t ub_read_datetime(void)
{
  uint32_t datetime;
  int8_t er;
  uint8_t c, dmy[3], hms[3];
  char * p;
  const static char gprmc[] PROGMEM = "GPRMC,";
  const static char fields[] PROGMEM = "\002\012";

  if (time_uptime < epoch_offset_useby) return(0);

  spt_rx_start();
  er = copy_fields(gprmc, fields);
  spt_rx_stop();

  if (er) {
    DBG(tx_puts("copy_fields returned "));
    DBG(tx_putdec(er));
    DBG(tx_puts("\r\n"));
    return(er);
  }
  DBG(tx_puts("buffer is: "));
  DBG(tx_puts(scan_buffer));
  DBG(tx_puts("\r\n"));
  p = scan_buffer;
  DBG(tx_puts("Calling csv_dateortime ("));
  DBG(tx_puts(p));
  DBG(tx_puts(", ...)="));
  c = csv_dateortime(p, hms);
  if (!c) return(UB_ER_PARSE);
  p += c;
  DBG(print3(hms));
  if ('.' == *p) p += 3;
  if (',' != *p) return(UB_ER_PARSE);
  p++;
  DBG(tx_puts("Calling csv_dateortime ("));
  DBG(tx_puts(p));
  DBG(tx_puts(", ...)"));
  c = csv_dateortime(p, dmy);
  if (!c) return(UB_ER_PARSE);
  p += c;
  if (',' != *p) return(UB_ER_PARSE);
  DBG(print3(dmy));
  DBG(tx_puts("\r\n"));
  datetime = cal_seconds(dmy, hms);
  if (!datetime) return(UB_ER_PARSE);
  cli();
  epoch_offset_useby = time_uptime;
  sei();
  epoch_offset = datetime - epoch_offset_useby;
  epoch_offset_useby += 3600;
  return(0);
}

int8_t ub_datetime(uint32_t * tp)
{
  if (! epoch_offset) return(UB_ER_NODATA);
  if (tp) *tp = epoch_offset + time_uptime;
  return(0);
}

