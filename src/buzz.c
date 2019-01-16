/*
  (C) 2019 Harold Tay LGPLv3
  Buzz the vibrator (not the piezo buzzer).  Uses PC2 (gp_a2).
 */
#include <avr/io.h>
#include "yield.h"

void buzz(void)
{
  DDRC |= _BV(PC2);
  PORTC |= _BV(PC2);
  ydelay(50);
  PORTC &= ~_BV(PC2);
}
