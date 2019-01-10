/* (C) 2019 Harold Tay LGPLv3 */
/*
  Just as cruise*() takes control of the thruster, depth takes
  over control of pitch_trim() to maintain depth.
  As before, caller must call sense() regularly for us.
 */

#include <avr/interrupt.h>
#include "sense.h"
#include "mma.h"
#include "pitch.h"
#include "timer.h"
#include "depth.h"

static int16_t depth_min, depth_max;
static uint8_t skip;

static void depth_control(void)
{
  int8_t trim;
  if (skip > 0) { skip--; return; }
  trim = 0;
  if (sensed.depth <= depth_min) trim += (PITCH_NOSE_DOWN);
  if (sensed.depth >= depth_max) trim += (PITCH_NOSE_UP);
  if (sensed.pitch > -1) trim += (PITCH_NOSE_DOWN);
  if (sensed.pitch < -8) trim += (PITCH_NOSE_UP);
  if (!trim) return;
  (void)pitch_trim(trim); /* XXX */
  skip = 1;  /* 1s between calls, so delay for 2 seconds */
}

static uint8_t registered = 0;

void depth_register(int16_t dmax, uint16_t dmin)
{
  cli();
  depth_min = dmin;
  depth_max = dmax;
  sei();
  if (!registered) {
    time_register_slow(depth_control);
    registered = 1;
    skip = 0;
  }
}

void depth_deregister(void)
{
  time_deregister_fast(depth_control);
  registered = 0;
}
