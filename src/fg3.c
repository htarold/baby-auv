/* (C) 2019 Harold Tay LGPLv3 */
/*
  Foreground tasks for pitch testing.
  Don't need GPS or RF.
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
#include "spt.h"
#include "bg.h"  /* pub_angles */
#include "mma.h"  /* mma_get/set */
#include "handydefs.h"
#include "ctrl.h"

uint8_t fg_buffer[64];

static int8_t fg_init(void)
{
  ctrl_init();
  return(0);
}

static void stabilise(int8_t i)
{
  for (; i > 0; i--) {
    bg_attitude();
    ydelay(50);
    bg_attitude();
    ydelay(50);
  }
}

#if 0
static int16_t inflect(void)
{
  int8_t i;
  int16_t old_sth, sth;
  bg_attitude();
  old_sth = pub_angles.sin_pitch;
  for ( ; ; ) {
    ydelay(5);
    bg_attitude();
    sth = pub_angles.sin_pitch;
    if (sth > old_sth) break;
    old_sth = sth;
  }
  for (i = 0; i < 10; i++) {
    bg_attitude();
    ydelay(20);
  }
  return(old_sth);
}

static void level(void)
{
  static int8_t percent;
  int8_t delta;

  bg_mma(percent);
  ydelay(50);
  delta = inflect();
  syslog_attr("delta_sth", delta);
  /* percent += (delta/8); */
  /* percent += (delta/16); */
  CONSTRAIN(percent, -100, 100);
  stabilise(5);
}
#endif

static void noseup(void)
{
  bg_mma(-100);
  stabilise(10);
}

static void waitfornoseup(void)
{
  for ( ; ; ) {
    if (pub_angles.sin_pitch > 600) break;
    ydelay(100);
    ydelay(100);
  }
  stabilise(5);
}

/*
  Test pitch control only.
 */

void fg_task(void)
{
  int8_t i, er;
  syslog_attr("fg_task", 2);
  fg_init();
  noseup();
  syslog_attr("fg_task", 1);
  waitfornoseup();
  syslog_attr("fg_task", 0);
  for (i = 0; i < 15; i++) {
    syslog_attr("fg_task_i", i);
    /* level(); */
    er = ctrl_combo(CTRL_CMD_PITCH|CTRL_CMD_TWIRL, 127);
    syslog_attr("ctrl_combo", er);
    stabilise(5);
    noseup();
  }
  for ( ; ; ) ;
}
