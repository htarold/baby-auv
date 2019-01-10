/* (C) 2019 Harold Tay LGPLv3 */
/*
  Foreground tasks to test swimming.
  No need to any RF code.
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

static void waitforpitch(int16_t max, int16_t min)
{
  for ( ; ; ) {
    tx_strlit("Waiting for pitch\r\n");
    if (pub_angles.sin_pitch <= max && pub_angles.sin_pitch >= min)
      break;
    ydelay(100);
  }
  tx_strlit("Wait done\r\n");
}

void fg_mission(void)
{
  int8_t i, er;
  uint32_t timeout;
#define HDG 127
#define SECS 10
#define DEPTH 100

  bg_mma(-100);
  waitforpitch(1024, 700);
  syslog_attr("Wait_Vertical", 0);    /* in the water */

  for (i = 0; ; yield(), i++) {
    er = ctrl_combo(CTRL_CMD_TWIRL|CTRL_CMD_PITCH|CTRL_CMD_THRUST, HDG);
    syslog_attr("ctrl_combo_er", er);
    if (!er) break;
    ydelay(100);
  }

  /*
    Start run
   */
  timeout = time_uptime + SECS;
  while (time_uptime < timeout) {
    er = ctrl_steady(HDG, DEPTH);
    if (er) break;
  }

  /*
    End of run
   */
  ctrl_thruster(0, 0);
  waitforpitch(1024, 700);
  syslog_attr("Wait_Vertical", 0);    /* Picked up, forced upright */
  bg_mma(-100);
  waitforpitch(200, -200);
  syslog_attr("Wait_Horizontal", 0);  /* Forced horizontal, wants to up upright */
}

void fg_task(void)
{
  fg_init();
  for ( ; ; ) {
    tx_strlit("Calling fg_mission\r\n");
    fg_mission();
    tx_strlit("Called fg_mission\r\n");
    yield();
  }
}
