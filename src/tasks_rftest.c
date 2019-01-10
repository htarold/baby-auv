/* (C) 2019 Harold Tay GPLv3 */
/*
  Modified from fg1.c to test rf.
  Foreground tasks include GPS and RF modem.
  When activated, gets GPS time if not already done; then gets
  GPS position (always), then sends out backlogged data.
  XXX Modem code not yet implemented.

  (ub_task functionality subsumed by this task).
 */

#include <util/crc16.h>
#include <string.h> /* for memcpy() */
#include "fg.h"
#include "ub.h"
#include "hc12.h"
#include "yield.h"
#include "syslog.h"
#include "pkt.h"
#include "time.h"
#include "pitch.h"
#include "spt.h"
#include "i2c.h"
#include "sel.h"

uint8_t fg_buffer[64];

static uint16_t fg_gps_timeout;

void fg_set_gps_timeout(uint16_t tmo)
{
  fg_gps_timeout = tmo;
}

static int8_t fg_init(void)
{
  fg_gps_timeout = 300;
  pitch_init();
  i2c_init();
  return(SEL_INIT);
}

static void send_position(void)
{
  struct position * pp;
  struct header * hp;
  uint16_t crc;
  int8_t er, i;

  hp = (void *)fg_buffer;
  pp = (void *)(fg_buffer + sizeof (*hp));

  hp->magic = PKT_MAGIC;
  hp->node_id = 1; /* XXX */
  hp->type = PKT_TYPE_POSN;
  pp->zero = 0;
  pp->ff = 0xff;
  er = ub_position(&pp->position);
  if (er)
    pp->position.lat = pp->position.lon = (nav_t)0;

  crc = 0;
  for (i = 0; i < sizeof(*hp) + sizeof(*pp); i++) {
    crc = _crc_xmodem_update(crc, fg_buffer[i]);
  }

  memcpy(fg_buffer + sizeof(*hp) + sizeof(*pp), &crc, 2);

  hc12_xmit(fg_buffer, sizeof(*hp) + sizeof(*pp) + 2);
}

static void fg_xmit_data(void)
{
  uint8_t i;
  int8_t er;
  /*
    Right now we just send out GPS coords.
   */
  er = hc12_xmit_start();
  syslog_attr("hc12_xmit_start", er);
  ydelay(5);

  for (i = 0; i < 30; i++) {
    send_position();
    ydelay(100);
  }
  hc12_xmit_stop();
  syslog_attr("hc12_xmit_stop", 0);
  syslog_attr("fg_xmit_data", 0);
}
static void fg_recv_data(void)
{
  /* XXX Nothing to do for now */
  syslog_attr("fg_recv_data", 0);
}

static void fg_do_gps(void)
{
  int8_t i;

  hc12_xmit_stop();
  hc12_recv_stop();
  for (i = 0; i < 100; i++) {
    int8_t er;
    er = ub_start();
    if (!er)
      er = ub_read_position();
    syslog_attr("ub_start_read_position", er);
    if (!er) break;
    er = ub_read_nrsats();
    syslog_attr("ub_read_nrsats", er);
    ydelay(100);
  }
}

void print_pcf8574_port(void)
{
  syslog_attr("pcf8574_port", pcf8574_port);
}

void fg_mission(void) { ; }
void fg_task(void)
{
  int8_t er;

  er = fg_init();
  if (er) panic("fg_init_failure", er);

  er = ub_start();
  if (er)  panic("ub_start_failure", er);

  for ( ; ; ) {
    static const char fg_state[] PROGMEM = "fg_state";
    syslog_attrpgm(fg_state, 0);
    fg_mission();                     /* in an external file */
    syslog_attrpgm(fg_state, 1);
    print_pcf8574_port();
    fg_do_gps();
    print_pcf8574_port();
    syslog_attrpgm(fg_state, 2);
    print_pcf8574_port();
    fg_xmit_data();
    print_pcf8574_port();
    syslog_attrpgm(fg_state, 3);
    print_pcf8574_port();
    fg_recv_data();
    print_pcf8574_port();
  }
}
