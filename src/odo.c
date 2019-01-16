/*
  (C) 2019 Harold Tay LGPLv3
  Odometer functions.  Actually performs dead reckoning,
  not only odometer readings (but "odo" is a shorter
  prefix).  Also estimates drift.

  Is a task, so will always run independently of everything
  else.  When queried, returns an estimate of position (struct
  nav_pt).

  Path planning is iterative: repeatedly call nav_rhumb(origin,
  dst).  The resultant vector is the track.  Estimate the time
  to travel, to get the drift vector.  track - drift = planned.
  Given planned, estimate new travel time.  Re-estimate drift
  vector, etc.
 */

#include "ee.h"
#include "ub.h"  /* for current time */
#include "syslog.h"
#include "nav.h"
#include "tude.h"
#include "thrust.h"
#include "imaths.h"
#include "odo.h"
#include "bg.h"

#undef DEBUG
#ifdef DEBUG
#include "tx.h"  /* debug */
#define DBG(x) x
#else
#define DBG(x) /* nothing */
#endif

static struct nav_pt origin;
static uint32_t origin_time;

/* struct cartesian { int32_t north, east; }; */
struct cartesian auv;                 /* units of revolutions */
struct odo odo;

static void revs_to_am(struct cartesian * cp)
{
  cp->north *= odo.calib1024;
  cp->north /= 1024;
  cp->east *= odo.calib1024;
  cp->east /= 1024;
}
static void am_to_revs(struct cartesian * cp)
{
  static int32_t recip;
  static uint16_t calib1024;
  if (calib1024 != odo.calib1024) {
    calib1024 = odo.calib1024;
    recip = (1024L * 1024)/ odo.calib1024;
  }
  cp->north *= recip; cp->north /= 1024;
  cp->east *= recip;  cp->east /= 1024;
}
static void nav_to_amcart(struct nav * np, struct cartesian * cp)
{
  cp->north = np->range * icos1024(np->heading);
  cp->north /= 1024;
  cp->east = np->range * isin1024(np->heading);
  cp->east /= 1024;
}

void odo_init(void)
{
  ee_load(&odo, &ee.odo, sizeof(odo));
  /*
    calib1024 will be around 300/1024 angular measures per rev.
   */
  if (odo.calib1024 < 50 || odo.calib1024 > 500)
    odo.calib1024 = 309;
  syslog_attr("odo_calib1024", odo.calib1024);
}

static int32_t accum_revs;
static int8_t present_heading;
static uint16_t old_revs;

static void flush_accumulated(void)
{
  int32_t tmp;
  tmp = accum_revs * icos1024(present_heading);
  tmp /= 1024;
  auv.north += tmp;
  tmp = accum_revs * isin1024(present_heading);
  tmp /= 1024;
  auv.east += tmp;
}
void odo_periodically(void)
{
  int16_t revs, delta_revs;

  delta_revs = 0;

  if (thrust_get_percent() > 0) {
    revs = thrust_get_revs_x2()/2;
    if (old_revs > 0 && revs > old_revs)
      delta_revs = revs - old_revs;
    old_revs = revs;
  }
  if (!delta_revs) return;

  /*
    If our heading has not changed since last time, keep adding
    to accumulator.  We can afford to reduce resolution a bit.
   */
  if (pub_angles.heading + 0  != present_heading
    && pub_angles.heading + 1 != present_heading
    && pub_angles.heading - 1 != present_heading ) {
    flush_accumulated();
    present_heading = pub_angles.heading;
    accum_revs = delta_revs;
  } else
    accum_revs += delta_revs;
}

/*
  Given drift velocity vector, using auv vector, estimate our
  position: position = auv + drift.
  Drift vector is in units of revs per 1024 seconds.
 */
void odo_position(struct nav_pt * p, struct cartesian * driftp)
{
  uint32_t elapsed;
  struct cartesian sum;

  ub_datetime(&elapsed);
  elapsed -= origin_time;
  flush_accumulated();
  sum = *driftp;  /* could be {0,0} */
  sum.north *= 1024;
  sum.north /= elapsed;
  sum.north += auv.north;

  sum.east *= 1024;
  sum.east /= elapsed;
  sum.east += auv.east;
  revs_to_am(&sum);

  p->lat = (sum.north*64) + origin.lat;
  /* XXX p->lat > +90deg or less than -90deg? */
  /*
    Simplified from:
    p->lon = (sum.east*64) * icos1024();
    p->lon /= 1024;
   */
  p->lon = sum.east * icos1024(p->lat>>24);
  p->lon /= 16;
  p->lon += origin.lon;
}

/*
  When a new GPS fix is obtained (also upon startup).
  Estimates drift (current).
 */
int8_t odo_start(struct nav_pt * fix)
{
  uint32_t now;

  if (ub_datetime(&now)) return(-1);
  origin = *fix;
  origin_time = now;
  auv.north = auv.east = 0;
  return(0);
}

/*
  Returns drift velocity due to current.
 */
int8_t odo_stop(struct nav_pt * fix, struct cartesian * driftp)
{
  uint32_t elapsed;
  struct nav corr1, corr2;
  static struct nav_pt reck1, reck2; /* XXX */
  static struct cartesian drift2;
  uint16_t eps;

  driftp->north = driftp->east = 0;

  if (ub_datetime(&elapsed)) return(-1);
  if (origin_time >= elapsed) return(-1);
  elapsed -= origin_time;

  /*
    Perturb odometer calibration to find smaller error.
    XXX nav_rhumb can return a range of -1.
   */

  eps = odo.calib1024/32;

#define MIN_DRIFT 20
  odo.calib1024 += eps;
  odo_position(&reck1, driftp);
  odo.calib1024 -= eps;
  corr1 = nav_rhumb(&reck1, fix);
  if (corr1.range < MIN_DRIFT) return(1);
  if (-1 == corr1.range) return(-1);

  odo.calib1024 -= eps;
  odo_position(&reck2, &drift2);
  odo.calib1024 += eps;
  corr2 = nav_rhumb(&reck2, fix);
  if (corr2.range < MIN_DRIFT) return(1);
  if (-1 == corr2.range) return(-1);

  if (corr1.range != corr2.range) {
    static uint8_t sometimes;
    eps /= 2;
    if (corr1.range > corr2.range) {
      odo.calib1024 -= eps;
      corr1 = corr2;
      *driftp = drift2;
    } else if (corr1.range < corr2.range)
      odo.calib1024 += eps;
    syslog_attr("odo_calib1024", odo.calib1024);
    sometimes++;
    if (0 == (sometimes & 0xf))
      ee_store(&odo, &ee.odo, sizeof(odo));
  }

  /* Convert to cartesian, still in angular measure */
  nav_to_amcart(&corr1, driftp);

  /* Convert from angular to revs */
  am_to_revs(driftp);

  /* Convert to velocity (recip reduces execution time) */
  {
    int32_t recip;
    recip = (1024L * 1024) / elapsed;
    driftp->north *= recip; driftp->north /= 1024;
    driftp->east *= recip;  driftp->east /= 1024;
  }

  return(0);
}
