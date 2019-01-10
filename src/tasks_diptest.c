/* (C) 2019 Harold Tay GPLv3 */
/*
  Test GPS behaviour in water.
 */
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include "time.h"
#include "syslog.h"
#include "yield.h"
#include "pitch.h"
#include "buzz.h"

void fg_mission(void)
{
  int8_t i;
  syslog_attr("diptest", 0);
  pitch_down();
  /* Test vibrator */
  buzz();
  /* 15 seconds under water */
  for (i = 0; i < 15; i++)
    ydelay(100);
  pitch_up();
  buzz();
  syslog_attr("diptest", 1);
}
