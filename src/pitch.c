/* (C) 2019 Harold Tay LGPLv3 */
#include "ee.h"
#include "pitch.h"
#include "mma.h"
#include "syslog.h"
#include "tude.h"
#include "time.h"
#include "handydefs.h"
#include "yield.h"

#define PITCH_RECALIBRATE

/*
  From nose-high, move to nose-level (or slightly below
  horizontal).

  mma convention:
  mma_set(-100) means mass moves to the rear, nose goes up.
  mma_set(100) means mass moves to forwards, nose goes down.
 */
static struct pitch pitch;
uint32_t pitch_recently;

static int8_t inflect(void)
{
  int8_t pitch, direction, i, er;
  struct angles angles;

  pitch_recently = time_uptime;
  direction = 0;
  ydelay(90);                         /* avoid actuation noise */
  er = tude_get(&angles, TUDE_RATE_FAST);
  if (er) goto done;
  /* -1024 < sin_pitch < 1024 */
  pitch = angles.sin_pitch/16;
  syslog_attr("inflect_pitch", pitch);

  for(i = 0; i < 9; i++) {            /* 3 seconds timeout */
    int16_t diff;
    ydelay(33);
    er = tude_get(&angles, TUDE_RATE_FAST);
    if (er) goto done;
    diff = angles.sin_pitch/16 - pitch;
    if (!(diff/2)) break;
    if (! direction) direction = diff;
    if (diff * direction <= 0) break; /* changed direction */
    pitch = angles.sin_pitch/16;
    syslog_attr("inflect_pitch", pitch);
  }
done:
  pitch_recently = time_uptime;
  return(er);
}

static void actuate(int8_t percent)
{
  pitch_recently = time_uptime;
  mma_set(percent);
  syslog_attr("pitch_mma", percent);
}

/*
  XXX Only works from nose-up to level.
 */
int8_t pitch_level(void)
{
  int8_t undershot, rebound, er;
  struct angles angles;

  actuate(pitch.act1);
  er = inflect();
  if (er) return(er);
  er = tude_get(&angles, TUDE_RATE_FAST);
  if (er) return(er);

  undershot = angles.sin_pitch/16;
  syslog_attr("undershot", undershot);

  actuate(pitch.act2);
  er = inflect();
  if (er) return(er);
  er = tude_get(&angles, TUDE_RATE_FAST);
  if (er) return(er);
  rebound = angles.sin_pitch/16;
  syslog_attr("rebound", rebound);

  /* Aim for slight negative pitch, rather than exactly 0 */
  undershot += 2;

  /*
    Do 2 things: drive undershot to 0; and make rebound ==
    undershot (i.e. no oscillation).
   */

  if (undershot != 0) {
    pitch.act1 += (undershot/8);
    CONSTRAIN(pitch.act1, -100, 100);
    syslog_attr("pitch_act1", pitch.act1);
  }

  /*
    act2 cannot be smaller than act1; i.e. act2 must act to push
    the nose DOWN, not UP.
   */
  if (rebound != undershot) {
    pitch.act2 += (rebound - undershot)/4;
    CONSTRAIN(pitch.act2, -100, 100);
    if (pitch.act2 < pitch.act1)
      pitch.act2 = pitch.act1;
    syslog_attr("pitch_act2", pitch.act2);
  }

  EE_MAYBE {
    ee_store(&pitch, &ee.pitch, sizeof(ee.pitch));
    syslog_puts("\nee.pitch stored\n");
  }

  return(0);
}

static int8_t pitch_to(int8_t extreme /* +/-100 */)
{
  int8_t here;
  here = mma_get();
  actuate(here/2 + extreme/2);
  inflect();
  actuate(extreme);
  inflect();
  return(0);
}

int8_t pitch_up(void) { return(pitch_to(-100)); }
int8_t pitch_down(void) { return(pitch_to(100)); }

/*
  This is the only pitch_* function safe to call from ISR.
 */
int8_t pitch_trim(int8_t inc)
{
  CONSTRAIN(inc, -3, 3);
  pitch_recently = time_uptime;
  return(mma_inc(inc));
}

void pitch_init(void)
{
#ifdef PITCH_RECALIBRATE
  ;
#else
  if (ee_load(&pitch, &ee.pitch, sizeof(ee.pitch)))
#endif
  {
    pitch.act1 = -20;
    pitch.act2 = 0;
  }
  mma_init();
}
