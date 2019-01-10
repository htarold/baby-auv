/* (C) 2019 Harold Tay GPLv3 */
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "time.h"
#include "tude.h"
#include "syslog.h"
#include "tx.h"
#include "yield.h"
#include "ctrl.h"

static struct angles angles;

static void delay(void)
{
  int8_t i;
  for (i = 8; i > 0; i--)
    ydelay(100);
}

static void waitforpitchup(void)
{
  static const char s[] PROGMEM = " Waiting";
  syslog_putpgm(s);
  for( ; ; ) {
    tude_get(&angles, TUDE_RATE_FAST);
    if (angles.sin_pitch > 707) break; /* about 45 degrees */
    ydelay(100);
  }
}

void fg_mission(void)
{
  int8_t i, er;

  ydelay(100);

  syslog_attr("ctrl_combo", 0);
  er = ctrl_combo(0, 0);
  if (er) syslog_attr("ctrl_er", er);
  waitforpitchup();

  tx_puts("Ready!\r\n");
  ydelay(100);  /* 1 second */
  ydelay(100);  /* 1 second */
  ydelay(100);  /* 1 second */
  ydelay(100);  /* 1 second */

  for (i = 0; i < 20; i++) {
    syslog_puts(" Nose_Level\n");
    syslog_attr("ctrl_combo", CTRL_CMD_PITCH|CTRL_CMD_TWIRL);
    er = ctrl_combo(CTRL_CMD_PITCH|CTRL_CMD_TWIRL, -25);
    if (er) syslog_attr("ctrl_er", er);
    delay();
    syslog_attr("ctrl_combo", 0);
    syslog_puts(" Nose_Up\n");
    er = ctrl_combo(0, 0);
    if (er) syslog_attr("ctrl_er", er);
    waitforpitchup();
    delay();
  }

  for( ; ; ) {
    syslog_puts(" Stopping\n");
    ydelay(100);
  }
}
