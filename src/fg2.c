/*
  (C) 2019 Harold Tay LGPLv3
  Alternate foreground task, for use in tracker.
  Hardware has been modified to keep GPS powered on all the time.
  I.e. switching will not turn off GPS, but WILL turn off HC12.
  spt is also switched.

  Top line (line 0) is for latest valid received AUV position.
  Bottom line first word (3 chars plus space) is the age of that
  received position, in seconds or minutes.
  Remainder of bottom line is GPS related: if fix obtained,
  shows heading and range to AUV.  If '?' shows, an update was
  received that did not have the GPS coordinates (AUV surfaced
  but was unable to get a fix).
 */

#include <util/crc16.h>
#include <string.h>
#include <avr/pgmspace.h>
#include "ub.h"
#include "hc12.h"
#include "sel.h"
#include "yield.h"
#include "syslog.h"
#include "pkt.h"
#include "lcd1602.h"
#include "time.h"
#include "tx.h"
#include "fmt.h"
#include "nav.h"
#include "spt.h"
#include "bat.h"
#include "fg.h"
#include "math.h"

uint8_t fg_buffer[FG_BUFFER_SIZE];

static uint32_t fix_time, packet_time;
static struct nav_pt our_position;
static struct nav_pt auv_position;
static uint8_t auv_position_valid;

static uint8_t column;
static char last_char;
static void put(char ch)
{
  if (column == 16) lcd_goto(1, 0);
  lcd_put(last_char = ch);
  column++;
}

#define LCD_PRINTLIT(lit) \
{ static const char s[] PROGMEM = lit; uint8_t i; \
for (i = 0; i < sizeof(s)-1; i++) put(pgm_read_byte_near(s + i)); }

static int8_t get_gps(void)
{
  static uint8_t got_time;
  uint32_t t;
  int8_t er;
  uint8_t i;

  hc12_xmit_stop();
  hc12_recv_stop();
  ub_start();

  if (got_time) goto get_position;

  for (i = 0; ; i++, yield()) {
    if (i >= 10) return(-1);
    er = ub_read_datetime();
    if (!er) break;
  }
  ub_datetime(&t);
  syslog_lattr("now", t);

get_position:

  /* Got time, now get position */
  for (i = 0; ; i++, yield()) {
    if (i >= 10) return(-1);
    er = ub_read_position();
    if (!er) break;
  }
  er = ub_position(&our_position);
  if (er) {
    syslog_attr("ub_position", er);
  } else {
    syslog_lattr("my_lat", our_position.lat);
    syslog_lattr("my_lon", our_position.lon);
    fix_time = time_uptime;
  }
  return(er);
}

static uint16_t get_char(void)
{
  uint8_t i;
  uint8_t ch;

  /*
    We should wait for around 2 seconds before timing out.
   */
  for (i = 0; ; i++, yield()) {
    if (i >= 200) return(0xffff);
    if (cbuf_haschar(spt_rx)) break;
  }
  ch = cbuf_get(spt_rx);
  return((uint16_t)ch);
}

static int8_t get_packet(void)
{
  int8_t er;
  uint16_t id;
  uint32_t timeout;
  struct nav_pt n;

  /* GPS power is not actually cut off, hardware has been hacked */
  ub_stop();
  
  hc12_recv_start();

  for(timeout = time_uptime + 15; ; ) {
    if (time_uptime > timeout) return(-1);
    er = pkt_read(fg_buffer, &id, get_char);
    syslog_attr("pkt_read", er);
    if (er == PKT_TYPE_POSN) break;
  }

  memcpy(&n, fg_buffer, sizeof(n));
  packet_time = time_uptime;
  if (NAV_IS_NULL(n)) {
    /* Keep old auv_position */
    auv_position_valid = 0;
    syslog_putlit(" E ");
  } else {
    auv_position = n;
    auv_position_valid = 1;
    syslog_lattr("auv_lat", auv_position.lat);
    syslog_lattr("auv_lon", auv_position.lon);
    syslog_putlit(" I ");
  }
  syslog_attr("auv_id", id);
  return(0);
}

static void print_decdeg(nav_t navt)
{
  int8_t negative, i;
  double decdegf;
  uint16_t deg;
  uint32_t dec;
  char * p;

  navt /= 64;
  if (navt < 0) {
    negative = 1;
    navt = 0 - navt;
  } else
    negative = 0;

  /*
  decdeg = 360.0*10000.0*navt / (1L<<26);
  decdeg = 0.05364418*navt
  Or:
  decdeg = (28125/524288)*navt
  Or:
  decdeg = (879/16384)*navt
   */

  /*
    Using 5 decimal places -> worst case (negative sign) uses 18
    chars!  This means up to 2 decimal places of longitude may
    be obscured!
   */
  decdegf = (360.0/67108864.0)*(float)navt;
  deg = (uint16_t)decdegf;
  dec = (uint32_t)((decdegf - deg) * 100000.0);
  syslog_attr("deg", deg);
  syslog_lattr("dec", dec);

  p = (char *)(fg_buffer + sizeof(fg_buffer) - 1);
  *p-- = '\0';

  /* Do decimal part first, 5 chars */
  for (i = 0; i < 5; i++) {
    *p-- = '0' + (dec%10);
    dec /= 10;
  }

  /* No decimal point, save space */
  *p-- = '.';
  /* Do degrees */
  do { *p-- = '0' + (deg%10); } while (deg /= 10);

  if (negative) *p = '-';
  else p++;

  /* tx_puts(p); */
  for ( ; *p; p++) {
    put(*p);
  }
}

static void print_latlon(void)
{
  print_decdeg(auv_position.lat);
  put(' ');
  yield();
  print_decdeg(auv_position.lon);
  yield();
}

static void printi(int16_t i)
{
  char * p;
  for (p = fmt_i16d(i); *p; p++)
    put(*p);
}
static void printl(int32_t i)
{
  char * p;
  for (p = fmt_i32d(i); *p; p++)
    put(*p);
}

static void print_age(void)
{
  int16_t age;
  char units;
  if (!packet_time) age = -1;
  else age = time_uptime - packet_time;
  if (age < 60) {
    units = 's';
  } else {
    units = 'M';
    age /= 60;
  }
  printi(age);
  put(units);
}

static void print_all(void)
{
  struct nav n;
  uint16_t hdg;
  uint32_t range;

  column = 0;
  lcd_goto(0, 0);
  print_latlon();
  while (column < 15) put(' ');
  if (last_char != ' ') put(' ');
  print_age();
  n = nav_rhumb(&our_position, &auv_position);
  /* convert to degrees */
  hdg = ((uint8_t)n.heading * (360/8))/(256/8);
  printi(hdg);
#define DEGREES_SYMBOL 0xDF
  if (!auv_position_valid) put('?');
  else put(DEGREES_SYMBOL);
  /* Convert to metres */
  range = (n.range * 19) / 32;
  printl(range);
  put('m');
  if (time_uptime - fix_time > 30) put('*');
  if (!auv_position_valid) put('?');
  lcd_clrtoeol();
}

void fg_task(void)
{
  uint8_t c;

  tx_strlit("Entered arf_task\r\n");
  lcd_init();
  lcd_backlight(1);
  lcd_newline(0);
  LCD_PRINTLIT("Starting...");
  lcd_clrtoeol();
  time_init();

  for (c = 0; ; c++) {
    int8_t er;
    uint16_t mvolts;

    mvolts = bat_voltage();
    syslog_attr("mv", mvolts);

    er = get_gps();
    syslog_attr("get_gps", er);
    er = get_packet();
    syslog_attr("get_packet", er);
    print_all();
  }
}
