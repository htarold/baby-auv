/*
  (C) 2019 Harold Tay LGPLv3
  Foreground tasks to test swimming back and forth until
  interrupted by user.
  No need for any RF or GPS code.
 */

#include <util/crc16.h>
#include <string.h> /* for memcpy() */
#include "fg.h"
#include "yield.h"
#include "syslog.h"
#include "time.h"
#include "bg.h"  /* pub_angles */
#include "mma.h"  /* mma_get/set */
#include "ctrl.h"
#include "tx.h"

uint8_t fg_buffer[64];

static int8_t fg_init(void)
{
  ctrl_init();
  return(0);
}

static uint8_t mission_hdg;
static int8_t mission_duration;

void fg_mission(void)
{
  int8_t i, er;
  uint32_t timeout;
#define DEPTH 100

  bg_mma(-100);
  syslog_attr("fg_mission_start_hdg", mission_hdg);
  syslog_attr("fg_mission_duration", mission_duration);

  /*
    Wait 40 seconds on surface, delaying if user interrupts run.
   */
  i = 0;
  while (i < 40) {
    if (pub_angles.sin_pitch > 700) {
      i += 4;
      syslog_attr("Run_Counting", i);
    } else {
      i = 0;
      syslog_attr("Run_Delayed", 0);
    }
    ydelay(100);
    ydelay(100);
    ydelay(100);
    ydelay(100);
  }

  for (i = 0; ; i++) {
    er = ctrl_combo(CTRL_CMD_TWIRL|CTRL_CMD_PITCH|CTRL_CMD_THRUST, mission_hdg);
    syslog_attr("ctrl_combo_er", er);
    if (!er) break;
    ydelay(100);
  }

  /*
    Start run
   */
  timeout = time_uptime + mission_duration;
  while (time_uptime < timeout) {
    er = ctrl_steady(mission_hdg, DEPTH);
    if (er) {
      syslog_attr("ctrl_steady", er);
      break;
    }
  }

  /*
    End of run
   */
  ctrl_thruster(0, 0);
  bg_mma(-100);
  syslog_attr("fg_mission_done_hdg", mission_hdg);
}

void fg_task(void)
{
  fg_init();
  for ( ; ; ) {
    /* Go NE (45 degrees) */
    mission_hdg = 32;
    mission_duration = 22;
    fg_mission();
    yield();
    /* Gp SW (180 degrees opposite) */
    mission_hdg = 32 + 128;
    mission_duration = 22;
    fg_mission();
    yield();
  }
}
