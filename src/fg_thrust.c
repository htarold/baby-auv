/*
  (C) 2019 Harold Tay LGPLv3
  Fake task to test bg1's response to thruster.
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

void fg_task(void)
{
  int8_t i;

  fg_init();
  for ( ; ; ) {
    ctrl_thruster(15, 0);
    for (i = 0; i < 20; i++)
      ydelay(100);
    ctrl_thruster(25, 0);
    for (i = 0; i < 20; i++)
      ydelay(100);
    ctrl_thruster(50, 0);
    for (i = 0; i < 20; i++)
      ydelay(100);
    ctrl_thruster(100, 0);
    for (i = 0; i < 20; i++)
      ydelay(100);
    ctrl_thruster(50, 0);
    for (i = 0; i < 20; i++)
      ydelay(100);
    ctrl_thruster(25, 0);
    for (i = 0; i < 20; i++)
      ydelay(100);
    ctrl_thruster(15, 0);
    for (i = 0; i < 20; i++)
      ydelay(100);
    ctrl_thruster(0, 0);
    for (i = 0; i < 20; i++)
      ydelay(100);
  }
}
