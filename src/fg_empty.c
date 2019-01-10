/* (C) 2019 Harold Tay LGPLv3 */
/*
  Fake task to test bg1's response to thruster.
 */

#include <util/crc16.h>
#include <string.h> /* for memcpy() */
#include "fg.h"
#include "yield.h"
#include "syslog.h"
#include "time.h"
#include "bg.h"  /* pub_angles */

uint8_t fg_buffer[64];

static int8_t fg_init(void)
{
  return(0);
}

void fg_task(void)
{
  fg_init();
  for ( ; ; ) {
    yield();
  }
}
