/* (C) 2019 Harold Tay GPLv3 */
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "time.h"
#include "syslog.h"
#include "yield.h"
#include "repl.h"
#include "ctrl.h"
#include "bg.h"
#include "ub.h"

static struct nav_pt here;            /* current position */
static struct waypoint home;          /* last waypoint */

static struct nav check_distance(struct nav_pt * fr, struct nav_pt * to)
{
  struct nav n;
  n = nav_rhumb(fr, to);
  syslog_attr("chk_line", repl_line_number());
  syslog_lattr("chk_fr_lat", fr->lat);
  syslog_lattr("chk_fr_lon", fr->lon);
  syslog_lattr("chk_to_lat", to->lat);
  syslog_lattr("chk_to_lon", to->lon);
  syslog_attr("chk_range", n.range);
  syslog_attr("chk_heading", n.heading);
  if (n.range > 1000)
    panic("Die:Waypt_too_Far_line", repl_line_number());
  return(n);
}

static int8_t get_next_waypoint(struct waypoint * wp)
{
  int8_t ret;
  do {
    ret = repl_next(wp);
    syslog_attr("repl_next", ret);
    if (ret < 0) panic("Err:Repl_Next", ret);
    if (ret == REPL_WAYPOINT || ret == REPL_END) {
      if (!NAV_IS_NULL(wp->n)) {
        syslog_lattr("wp_lat", wp->n.lat);
        syslog_lattr("wp_lon", wp->n.lon);
        syslog_lattr("wp_until", wp->until);
        syslog_attr("wp_rate", wp->sample_rate);
        syslog_attr("wp_depth", wp->depth_cm);
      }
    }
  } while (REPL_AUVID == ret);
  return(ret);                  /* Either REPL_WAYPOINT or REPL_END */
}

static uint16_t nr_waypoints;

/*
  Runs before mission, ok to panic if it fails.
 */
static void check_script(void)
{
  int8_t er, end;
  struct waypoint tmp_wp;
  struct nav_pt prev;

  /*
    Preliminary mission script check.  Cannot check distance between
    waypoints for 1st waypoint (because no current GPS location).
    Caller will check 1st waypoint later.
   */
  er = repl_init();
  if (er) panic("Err:Repl_Init1", er);

  NAV_MAKE_NULL(here);

  /*
    Check distance between waypoints; also, save last waypoint
    as home.
   */

  NAV_MAKE_NULL(prev);

  do {
    end = get_next_waypoint(&tmp_wp);
    nr_waypoints++;

    if (!NAV_IS_NULL(tmp_wp.n)) {
      home = tmp_wp;
      if (!NAV_IS_NULL(prev))
        (void)check_distance(&prev, &tmp_wp.n);
      prev = tmp_wp.n;
    }
  } while (REPL_END != end);

  er = repl_init();
  if (er) panic("Err:Repl_Init2", er);
}

static int8_t retry_ctrl_combo(uint8_t cmd, int8_t hdg)
{
  int8_t retries, er;
  for (retries = 0; retries < 12; retries++) {
    er = ctrl_combo(cmd, hdg);
    if (!er) return(0);
    syslog_attr("ctrl_combo", er);
    if (retries > 6)
      syslog_attr("Ctrl_combo_Retries", retries);
    ydelay(100);
    ydelay(100);
    ydelay(100);
    ydelay(100);
    ydelay(100);
  }
  return(er);
}

static void msn_descend(int16_t depth_cm)
{
  retry_ctrl_combo(0, 0);  /* pitch nose up */
  panic("Die:fn_Not_Impl", -99);
}

static void msn_surface(void)
{
  pub_t pub;

  retry_ctrl_combo(0, 0);             /* pitch up, all stop */
  PUB_T_RESET(pub);

  for ( ; ; yield()) {                /* XXX Assumes trimmed +ve */
    if (!PUB_REFRESHED(PUB_CM, pub))
      continue;
    if (pub_cm < 90)
      break;
  }
}


/*
  If this fails, just return: fg_mission will retry the same
  waypoint, which is the correct action.
 */
static void msn_swim(struct waypoint * wp)
{
  struct nav nav;
  uint32_t timeout;
  int8_t er;
  uint16_t secs;

  nav = nav_rhumb(&here, &(wp->n));
  syslog_attr("swim_range", nav.range);
  syslog_attr("swim_hdg", nav.heading);
  if (nav.range < 10) {               /* Nothing to do, already there */
    ydelay(100);
    ydelay(100);
    ydelay(100);
    return;
  }

  /* Assumes speed is about 1m/s */
  secs = (nav.range>60?60:nav.range);
  er = retry_ctrl_combo(CTRL_CMD_TWIRL|CTRL_CMD_PITCH|CTRL_CMD_THRUST,
    nav.heading);
  if (er) return;

  for (timeout = time_uptime + secs; timeout > time_uptime; yield()) {
    er = ctrl_steady(nav.heading, wp->depth_cm);
    syslog_attr("ctrl_steady", er);
    if (!er) continue;
    break;
  }
  (void)retry_ctrl_combo(0, 0);
}

static struct waypoint wpt;           /* current waypoint */

static uint8_t is_departure_time(void)
{
  uint32_t now;
  int8_t er;
  if (!wpt.until) return(1);
  er = ub_datetime(&now);
  if (er) {
    syslog_attr("is_departure_time", er);
    return(1);  /* XXX Leave now */
  }
  return(now >= wpt.until);
}

static int8_t get_gps_fix(struct nav_pt * p)
{
  int8_t er;
  er = ub_position(p);
  syslog_attr("get_gps_fix", er);
  return(er);
}

void fg_mission(void)
{
  static uint8_t flag_initialised;
  int16_t i;
  struct nav n;
  static const char me[] PROGMEM = "fg_mission";

  if (!flag_initialised) {
    syslog_attrpgm(me, 9);
    (void)ctrl_combo(0, 0);           /* Expect CTRL_ERR_POSE */
    check_script();
    NAV_MAKE_NULL(wpt.n);
    flag_initialised = 1;
  } else
    syslog_attrpgm(me, 8);

  if (0 != get_gps_fix(&here)) {      /* XXX infinite loop */
    syslog_attrpgm(me, 7);
    return;
  }
  syslog_attrpgm(me, 6);

  syslog_attr("nr_waypoints", nr_waypoints);

  if (!NAV_IS_NULL(wpt.n)) {          /* have current waypoint */
    syslog_attrpgm(me, 5);
    n = nav_rhumb(&here, &wpt.n);
#define METRES /0.596
    if (n.range < (8 METRES) && (nr_waypoints > 1)) {
      /* Arrived, mark done */
      repl_done(&wpt, REPL_DONE_OK);
      nr_waypoints--;
      NAV_MAKE_NULL(wpt.n);
    }
  }
    
  if (NAV_IS_NULL(wpt.n)) {
    syslog_attrpgm(me, 4);
    (void)get_next_waypoint(&wpt);    /* errors already handled */
  }

  /* wait for pitch up, indicates start of mission */
  if (1 == flag_initialised) {
    syslog_attrpgm(me, 3);
    for (i = 0; ; i++) {
      if (i >= 20) return;
      syslog_attr("Gps_Ready", pub_angles.sin_pitch);
      if (pub_angles.sin_pitch > 510) break;
      ydelay(100);
      ydelay(100);
    }
    flag_initialised = 2;
  }

  syslog_attrpgm(me, 2);
  msn_swim(&wpt);
  syslog_attrpgm(me, 1);
  msn_surface();
  syslog_attrpgm(me, 0);
}
