/* (C) 2019 Harold Tay LGPLv3 */
/*
  Moving mass actuator.
  mma convention:
  mma_set(-100) means mass moves to the rear, nose goes up.
  mma_set(100) means mass moves to forwards, nose goes down.
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <stdint.h>
#include "ppm.h"
#include "handydefs.h"
#include "ee.h"
#include "syslog.h"
#include "mma.h"

#define REVERSED servo.reversed
#define CENTRE servo.centre
#define THROW servo.throw
struct servo servo;  /* give calibrator access to config */

static int8_t position;  /* +/- 100, canonical unreversed position */

void mma_init(void)
{
  if (ee_load(&servo, &ee.servo, sizeof(ee.servo))) {
    REVERSED = 0;
    CENTRE = 225;
    THROW = 116;
  }
  CONSTRAIN(REVERSED, 0, 1);
  CONSTRAIN(CENTRE, 200, 250);
  CONSTRAIN(THROW, 0, 117);
  ppm_init();
}

int8_t mma_get(void) { return(position); }

/*
  us = centre*6 +/- 6*throw*position/100
  6/100 = 0.00001111...b
 */
static void convert_to_us(void)
{
  int16_t us;
  us = THROW * position;
  if (REVERSED) us = 0 - us;
  us /= 32;
  us += us/2 + us/4 + us/8 + us/16;
  us += CENTRE*6;
  ppm_set(us);
}
void mma_set(int8_t pos)
{
  CONSTRAIN(pos, -100, 100);
  position = pos;
  convert_to_us();
}

int8_t mma_inc(int8_t delta)
{
  int16_t newpos;
  int8_t rangerr;

  newpos = (int16_t)position + delta;
  rangerr = -1;
  if (newpos > 100) newpos = 100;
  else if (newpos < -100) newpos = -100;
  else rangerr = 0;
  position = newpos;
  convert_to_us();
  return(rangerr);
}

void mma_syslog(void)
{
  static int8_t old_position = 127;
  if (old_position == position) return;
  syslog_attr("mma", old_position = position);
}
