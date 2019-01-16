/*
  (C) 2019 Harold Tay LGPLv3
  Controllers are responsible for a particular behaviour.
  Some controllers are called synchronously, some are persistent
  (basically they take over the task).  More than one controller
  can be active at a time.

  This module "owns" pitch and thruster.
 */
#include <stdint.h>
#include "yield.h"
#include "tude.h"
#include "thrust.h"
#include "ee.h"
#include "syslog.h"
#include "handydefs.h"
#include "bg.h"
#include "ctrl.h"
#include "mma.h"
#include <avr/io.h>

#undef ENABLE_TWIRL_OFFSET
#define DEGREES *16*32/45
#define STH_NEUTRAL (-5 DEGREES)

#define TWIRL_THRUST_LEVEL -35        /* likely to be -20 to -30 */

static struct twirl twirl;            /* Calibration offsets */
static struct pitch pitch;

void ctrl_thruster(int8_t percent, int8_t walk)
{
  thrust_set(percent, walk);
  syslog_attr("thrus", percent);
  syslog_attr("wlk", walk);
}

/*
  Nose up, twirl, pitch level, begin thrust, or any subset.
  (Because this does not factor well into separate functions.)
 */

static uint8_t curvedn(int16_t new, int16_t old)
{
  syslog_attr("curve_new", new);
  syslog_attr("curve_old", old);
  syslog_attr("curve_dn", new <= old);
  return(new <= old);
}
static uint8_t curveup(int16_t new, int16_t old)
{
  syslog_attr("curve_new", new);
  syslog_attr("curve_old", old);
  syslog_attr("curve_up", new >= old);
  return(new >= old);
}

static int8_t inflect(int8_t * p, uint8_t (*curve)(int16_t, int16_t))
{
  int8_t i, er;
  int16_t old_sin_pitch;

  /*
    Caller has delayed past noise and false inflections.
   */
  er = bg_attitude();
  if (er) return(CTRL_ERR_TUDE);
  old_sin_pitch = pub_angles.sin_pitch;

  for (i = 9; i > 0; i--) {           /* time out in 3 seconds */
    ydelay(33);
    er = bg_attitude();
    if (er) return(CTRL_ERR_TUDE);
    if (curve(pub_angles.sin_pitch, old_sin_pitch)) break;
    old_sin_pitch = pub_angles.sin_pitch;
  }
  if (p) *p = pub_angles.sin_pitch / 16;
  return(0);
}

int8_t ctrl_combo(uint8_t cmd, int8_t hdg)
{
  uint8_t i;
  int8_t er, percent, p;

  syslog_attr("ctrl_combo_cmd", cmd);
  syslog_attr("ctrl_combo_hdg", hdg);
  ctrl_thruster(0, 0);
  percent = mma_get();
  if (percent != -100)                /* not pointing straight up */
    bg_mma(-100);

  ydelay(100);
  if (inflect(&p, curvedn))
    return(CTRL_ERR_TUDE);

  if (p < 706/16)                     /* < 45degrees */
    return(CTRL_ERR_POSE);

  /* Now pointing up */

  if (cmd & CTRL_CMD_TWIRL) {
    int8_t target;
    target = hdg - twirl.offset;
    syslog_attr("target", target);
    ctrl_thruster(TWIRL_THRUST_LEVEL, 0); /* XXX */
    /* Get some depth, settle to stable orbit */
    ydelay(100);
    ydelay(100);
    ydelay(100);
#define TWIRL_DELAY 20
#define SECONDS * 100 / TWIRL_DELAY
    for (i = 45 SECONDS; i > 0; i--, ydelay(TWIRL_DELAY)) {
      int8_t d;
      er = CTRL_ERR_TUDE;
      if (bg_attitude()) break;
      er = 0;
      d = pub_angles.heading - target;
      if (d > -10 && d < 10) break;
      er = CTRL_ERR_TIMEOUT;
    }

    ctrl_thruster(0, 0);
    if (er) return(er);

#if ENABLE_TWIRL_OFFSET  /* Walks are very effective. */
    {
    /*
      aiming completed, adapt twirl...
     */
      int8_t hdg_error;
      er = CTRL_ERR_TUDE;
      if (bg_attitude()) goto bomb;
      hdg_error = (int8_t)pub_angles.heading - hdg;
      syslog_attr("hdg_err", hdg_error);
      twirl.offset += hdg_error / 2;
      syslog_attr("twirl_offset", twirl.offset);

      if (hdg_error > 18 || hdg_error < -18)
      return(CTRL_ERR_AIM);
      /* EE_MAYBE */ {
        ee_store(&twirl, &ee.twirl, sizeof(twirl));
        syslog_attr("twirl_stored", 1);
      }
    }
#endif /* ENABLE_TWIRL_OFFSET */
  }

  /*
    Then pitch level
   */
  if (cmd & CTRL_CMD_PITCH) {
    int8_t undershot, rebound;

    bg_mma(pitch.act1);
    ydelay(100);                      /* Avoid actuation noise... */
    ydelay(100);                      /* and false inflections */
    if (inflect(&undershot, curveup)) goto bomb;
    syslog_attr("undershot", undershot);

    /* Set a bias so it ends up slightly negative */
    undershot -= (int8_t)(STH_NEUTRAL/16);

    /* Adapt act1.  Avoid large corrections. */
    if (undershot) {
      CONSTRAIN(undershot, -16, 16);
      pitch.act1 += undershot/4;      /* small prop. correction */
      if (undershot > 0) pitch.act1++;/* constant correction */
      else pitch.act1--;
    }
    syslog_attr("pitch_act1", pitch.act1);

    if (undershot > 1 || undershot < -11)
      return(CTRL_ERR_PITCH);

    if (pitch.act2 <= pitch.act1)
      pitch.act2 = pitch.act1 + 2;
    bg_mma(pitch.act2);

    if (cmd & CTRL_CMD_THRUST) {
      ctrl_thruster(100, 0);
      bg_mma(mma_get() + 2);          /* extra nose down to start */
    }
    ydelay(100);                      /* Avoid actuation noise */

    /*
      Can take up to 3 seconds for inflect() to return, which is
      very long if thrusting.
     */
    if (inflect(&rebound, curvedn)) goto bomb;
    syslog_attr("rebound", rebound);
    /* adapt act2 */
    pitch.act2 = pitch.act1 + (rebound - undershot);
    if (pitch.act2 <= pitch.act1)
      pitch.act2 = pitch.act1 + 2;
    syslog_attr("pitch_act2", pitch.act2);
    EE_MAYBE {
      ee_store(&pitch, &ee.pitch, sizeof(pitch));
      syslog_attr("store_pitch", 1);
    }
  }

  return(0);
bomb:
  ctrl_thruster(0, 0);
  return(CTRL_ERR_TUDE);
}


void ctrl_init(void)
{
#if ENABLE_TWIRL_OFFSET
  if (ee_load(&twirl, &ee.twirl, sizeof(twirl)))
    twirl.offset = 20;
#else
  twirl.offset = 0;
#endif
  if (ee_load(&pitch, &ee.pitch, sizeof(pitch))) {
    pitch.act1 = -15;
    pitch.act2 = 0;
  }
  mma_init();
}

/*
  XXX Who determines when to stop?  A single (successful) twirl
  should lead to at least 20metres/20seconds of travel.  If we
  let ctrl_steady() run without end, need a way to tell it to
  stop: i.e. it must be a task.  So _steady() will return on its
  own after a few seconds, and so must be called repeatedly.

  Thruster walk will only ever be active WHILE within this
  controller.
 */

static int8_t control_walk(uint8_t hdg_sp)
{
  int8_t dhdg, walk;
  /*
    If heading error too great, give up.  Could be due to
    collision for example.
   */

  dhdg = (uint8_t)pub_angles.heading - hdg_sp;
  if (dhdg > 32 || dhdg < -32)
    return(CTRL_ERR_POSE);
  if (dhdg > 0)      walk = THRUST_WALK_TURN_LEFT;
  else if (dhdg < 0) walk = THRUST_WALK_TURN_RIGHT;
  else               walk = 0;

  CONSTRAIN(walk,-1,1);

  ctrl_thruster(100, walk);
  return(0);
}

static int16_t vehicle_depth_correction(void)
{
#define VEHICLE_LENGTH_CM 55
  return 0 -
    (((VEHICLE_LENGTH_CM/2) * (pub_angles.sin_pitch/16)) / (1024/16));
}

/*
  Fast pitch control ('twitch').  Assumes current pitch is close
  to horizontal.
 */
static void twitch(int8_t inc)
{
  int8_t position;
  position = mma_get();
  bg_mma(position + (inc * 4));
  ydelay(20);
  bg_mma(position - (inc * 3));
  ydelay(20);
  bg_mma(position + inc);
  /* presumably there is no/minimal oscillation */
}

static int8_t control_depth(int16_t cm_sp)
{
  static int16_t neutral_pitch;
  int16_t cm, pitch_sp;
  const static char name_neutral[] PROGMEM = "neutral_pitch";

  /*
    Pitch which gives level flight.
    This is supposed to slowly converge.
   */
  if (!neutral_pitch) {
    neutral_pitch = STH_NEUTRAL;
    syslog_attrpgm(name_neutral, neutral_pitch);
  }

  if (pub_angles.sin_pitch > neutral_pitch + 150) return(-1);
  if (pub_angles.sin_pitch < neutral_pitch - 150) return(-1);

  cm = pub_cm + vehicle_depth_correction();

  pitch_sp = (cm - cm_sp)/2;
  CONSTRAIN(pitch_sp, -50, 50);
  pitch_sp += neutral_pitch;
  bg_pitch_set(pitch_sp, 20);
  return(0);
}

/* "Steady as she goes" */
int8_t ctrl_steady(uint8_t hdg_sp, int16_t cm_sp)
{
  int8_t er, i;
  pub_t pub;

  /*
    We have time to wait for sensors to poll as part
    of their normal routine.  Access to CTD must be managed, and
    access is also expensive.
   */

  PUB_T_RESET(pub);
  for (i = 80; ; i--) {  /* at least 2 seconds, refresh time */
    if (PUB_REFRESHED(PUB_ANGLES,pub)) break;
    if (i <= 0) { er = CTRL_ERR_TUDE; goto bomb; }
    ydelay(4);
  }

  er = control_walk(hdg_sp);
  yield();
  if (er) {
    syslog_attr("control_walk", er);
    goto bomb;
  }

  er = control_depth(cm_sp);
  yield();
  if (er) {
    syslog_attr("control_depth", er);
    goto bomb;
  }

  return(0);

bomb:
  ctrl_thruster(0, 0);                /* turn off walk */
  return(er);
}
