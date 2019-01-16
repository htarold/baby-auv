/*
  (C) 2019 Harold Tay LGPLv3
  Buzz the vibrator.  Uses PC3 (gp_a3).
  On for 1/2 second each call.
 */
#ifndef BUZZ_H
#define BUZZ_H

#include <avr/io.h>
#include "yield.h"

extern void buzz(void);

#endif /* BUZZ_H */
