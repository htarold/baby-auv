/* (C) 2019 Harold Tay LGPLv3 */
/*
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
#include "ctrl.h"
#include "spt.h"
#include "bg.h"  /* pub_angles */
#include "mma.h"  /* mma_get/set */

uint8_t fg_buffer[64];

static uint16_t fg_gps_timeout;

void fg_set_gps_timeout(uint16_t tmo)
{
  fg_gps_timeout = tmo;
}

static int8_t fg_init(void)
{
  fg_gps_timeout = 300;
  ctrl_init();
  return(0);
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
  if (er) {
    NAV_MAKE_NULL(pp->position);
  }

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
  /*
    Right now we just send out GPS coords.
   */
  bg_pitch_set(100, 0);
  hc12_xmit_start();
  syslog_attr("hc12_xmit_start", 0);  /* XXX */
  ydelay(5);

  for (i = 0; i < 12; i++) {
    send_position();
    ydelay(100);
  }
  hc12_xmit_stop();
  syslog_attr("hc12_xmit_stop", 0);  /* XXX */
  syslog_attr("fg_xmit_data", 0);
}
static void fg_recv_data(void)
{
  /* XXX Nothing to do for now */
  syslog_attr("fg_recv_data", 0);
}

static int8_t gps_error = 0;
static void post_gps_error(void)  /* to provide audio indication */
{
  syslog_attr("Gps_Error", gps_error);
}

static void fg_do_gps(void)
{
  uint32_t timeout;
  int8_t flag_need_time;
  struct nav_pt fix;

  hc12_xmit_stop();
  hc12_recv_stop();
  ub_start();

  timeout = time_uptime + fg_gps_timeout;
  flag_need_time = ub_datetime(0);
  bg_pitch_set(750, 80);

  for ( ; ; ) {
    int8_t nrsats;
    if (flag_need_time) {
      gps_error = ub_read_datetime();
      syslog_attr("ub_read_datetime", gps_error);
      if (!gps_error) {
        uint32_t now;
        flag_need_time = ub_datetime(&now);
        /* Synchronise our log */
        if (!flag_need_time)
          syslog_lattr("now", now);
      }
    }
    if (timeout < time_uptime) goto timeout;
    gps_error = ub_read_position();
    syslog_attr("ub_read_position", gps_error);
    if (!gps_error) break;
    post_gps_error();
    nrsats = ub_read_nrsats();
    gps_error = (nrsats>0?0:nrsats);
    syslog_attr("ub_read_nrsats", nrsats);
    if (timeout < time_uptime) goto timeout;
  }

  /*
    Don't call ub_stop(), let hc12 do it.
    GPS takes a long time to start up, if hc12 doesn't need the
    antenna, GPS should remain alive.
   */
  gps_error = ub_position(&fix);
  syslog_attr("ub_position", gps_error);
  if (!gps_error) {
    syslog_lattr("ub_lat", fix.lat);
    syslog_lattr("ub_lon", fix.lon);
  }
  spt_rx_stop();                      /* (we can reduce ISR load) */
  return;

timeout:
  syslog_attr("fg_do_gps", gps_error);
}

void fg_task(void)
{
  fg_init();
  for ( ; ; ) {
    static const char fg_state[] PROGMEM = "fg_state";
    syslog_attrpgm(fg_state, 0);
    fg_mission();                     /* in an external file */
    syslog_attrpgm(fg_state, 1);
    fg_do_gps();
    syslog_attrpgm(fg_state, 2);
    fg_xmit_data();
    syslog_attrpgm(fg_state, 3);
    fg_recv_data();
  }
}
