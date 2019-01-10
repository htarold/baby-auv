/* (C) 2019 Harold Tay GPLv3 */
/*
  Alternate RF task, for use in tracker.
  Hardware has been modified to keep GPS powered on all the time.
  I.e. switching will not turn off GPS, but will turn off HC12.
  spt is also switched.

  Top line (line 0) of display is for latest received AUV position.
  Bottom line first word (3 chars plus space) is the age of that
  received position, in seconds or minutes.
  Remainder of bottom line is GPS related: if fix obtained,
  shows heading and range to AUV.  Otherwise, shows GPS status.
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
#include "fg.h"                       /* only for fg_buffer[] */
uint8_t fg_buffer[FG_BUFFER_SIZE];

static uint32_t fix_time, packet_time;
static struct nav_pt our_position;
static struct nav_pt their_position;

#define LCD_PRINTLIT(lit) \
{ static const char s[] PROGMEM = lit; uint8_t i; \
for (i = 0; i < sizeof(s)-1; i++) lcd_put(pgm_read_byte_near(s + i)); }

static int8_t get_gps(void)
{
  uint32_t t;
  int8_t er;
  uint8_t i;

  hc12_xmit_stop();
  hc12_recv_stop();
  ub_start();
  for (i = 0; ; i++, yield()) {
    if (i >= 50) return(-1);
    er = ub_read_datetime();
    if (!er) break;
    lcd_goto(1, 12);
    if (i & 1) { LCD_PRINTLIT("GPS?");
    } else lcd_clrtoeol();
  }
  ub_datetime(&t);
  syslog_lattr("now", t);

  /* Got time, now get position */
  for (i = 0; ; i++, yield()) {
    if (i >= 50) return(-1);
    er = ub_read_position();
    if (!er) break;
    lcd_goto(1, 12);
    if (i & 1) { LCD_PRINTLIT("GPS?");
    } else lcd_clrtoeol();
  }
  er = ub_position(&our_position);
  if (er) {
    syslog_attr("ub_position", er);
  } else {
    syslog_lattr("lat", our_position.lat);
    syslog_lattr("lon", our_position.lon);
    fix_time = time_uptime;
  }
  return(er);
}

static uint8_t here;
static char rf_buffer[64];

static int8_t get_char(void)
{
  int8_t i;
  for (i = 0; ; i++) {
    if (i >= 40) return(-1);
    if (here >= sizeof(rf_buffer)-1) return(-1);
    if (cbuf_haschar(spt_rx)) break;
    yield();
  }
  rf_buffer[here++] = cbuf_get(spt_rx);
  return(0);
}

static int8_t get_header(void)
{
  int8_t i;
  for (i = 0; ; i++) {
    if (i >= 40) return(-1);
    here = 0;
    if (get_char()) goto bomb;
    if (rf_buffer[0] == PKT_MAGIC) break;
  }
  if (get_char()) goto bomb;
  if (get_char()) goto bomb;
  if (get_char()) goto bomb;
  return(0);
bomb:
  yield();
  return(-1);
}

static int8_t get_packet(void)
{
  int8_t i;
  uint16_t expected;
  uint16_t * crcp;

  ub_stop();
  hc12_recv_start();

  for (i = 0; ; i++) {
    int8_t j, save;
    if (i >= 10) return(-1);
    tx_strlit("## Calling get_header\r\n");
    if (get_header()) continue;
    save = here;
    /* Read 10 bytes (position structure) */
    for (j = 0; j < 10; j++) {
      if (get_char()) goto bad_char;
    }
    break;
bad_char: ;
    here = save;
  }
  /* Check CRC on packet */
  tx_strlit("## Check CRC\r\n");
  crcp = (void *)(rf_buffer + here);
  (void)get_char();
  (void)get_char();
  expected = 0;
  for (i = 0; i < sizeof(struct header) + sizeof(struct position); i++)
    expected = _crc_xmodem_update(expected, rf_buffer[i]);
  if (expected != *crcp) {
    tx_strlit("Bad CRC: expected");
    tx_putdec(expected);
    tx_strlit(" != ");
    tx_putdec(*crcp);
    tx_strlit(" received\r\n");
    return(-1);
  }
  packet_time = time_uptime;
  return(0);
}

static void print_decdeg(nav_t navt)
{
  /*
    = 360.0 * navt/(1<<26)
    = 180.0 * navt/(1<<25)
    = 90.0 * navt/(1<<24)
    = 45.0 * navt/(1<<23)
   */
  int8_t negative, i;
  uint16_t deg;
  uint32_t tmp, dec;
  char * p;

  if (navt < 0) {
    negative = 1;
    navt = 0UL - navt;
  } else
    negative = 0;

  tmp = navt * 45;
  deg = tmp >> 23;
  /*
    Lower 23 bits of tmp contain the fractional degrees.
    We only need 14 of those bits to get degree/16384
resolution.
   */
  tmp &= (1UL<<23)-1;
  tmp >>= 9;
  tmp &= (1<<14)-1;

  /*
    dec/16384 gives the fractional part.
    dec*10000/16384
    dec*5000/8192
    dec*2500/4096
    dec*1250/2048
    dec*625/1024
   */

  tmp *= 625;
  tmp /= 1024;
  dec = (uint16_t)tmp;
  p = (void *)(rf_buffer + sizeof(rf_buffer) - 1);
  *p-- = '\0';

  /* Do decimal part first, 4 chars */
  for (i = 0; i < 4; i++) {
    *p-- = '0' + (dec%10);
    dec /= 10;
  }

  /* No decimal point, save space */
  /* Do degrees */
  do { *p-- = '0' + (deg%10); } while (deg /= 10);
  if (negative) *p = '-';
  else p++;

  tx_puts(p);
  for ( ; *p; p++) lcd_put(*p);
}

static void print_latlon(void)
{
  print_decdeg(their_position.lat);
  lcd_put(' ');
  yield();
  print_decdeg(their_position.lon);
}

static void printi(int16_t i)
{
  char * p;
  for (p = fmt_i16d(i); *p; p++)
    lcd_put(*p);
}
static void printl(int32_t i)
{
  char * p;
  for (p = fmt_i32d(i); *p; p++)
    lcd_put(*p);
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
  lcd_put(units);
  lcd_put(' ');
}

void fg_task(void)
{
  uint8_t c;

  tx_strlit("Entered fg_task\r\n");
  lcd_init();
  lcd_backlight(1);
  lcd_newline(0);
  LCD_PRINTLIT("Starting...");
  lcd_clrtoeol();
  time_init();

  for (c = 0; ; c++) {
    int8_t er_gps, er_pkt;
    uint16_t mvolts;

    mvolts = bat_voltage();
    syslog_attr("mv", mvolts);

    er_gps = get_gps();
    er_pkt = get_packet();
    lcd_newline(0);
    if (!er_pkt)
      memcpy(&their_position, rf_buffer + 6, sizeof(their_position));
    print_latlon();                   /* top line */
    lcd_clrtoeol();
    lcd_newline(1);
    print_age();                      /* bottom line */
    if (!er_gps) {
      struct nav nav;
      uint16_t degrees;
      nav = nav_rhumb(&our_position, &their_position);
      /*
        Convert nav.range to meters, multiply by 0.596
	(= 19/32)
       */
      nav.range *= 19;
      nav.range /= 32;
      degrees = 45*nav.heading/32;
      printi(degrees);
      lcd_put(' ');
      printl(nav.range);
      lcd_put('m');
      syslog_attr("nav.heading", degrees);
      syslog_attr("nav.range", nav.range);
    } else {
      LCD_PRINTLIT("(No GPS)");
    }
    lcd_clrtoeol();
  }
}
