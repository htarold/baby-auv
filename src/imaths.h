#ifndef IMATHS_H
#define IMATHS_H

#include <stdint.h>

/*
  (C) 2019 Harold Tay LGPLv3
  Basic integer arithmetic.  256 slivs per circle.  Not only is
  this a convenient resolution, but also corresponds with
  compass precision.  See source file for attributions.
 */
/* Returns in whole degrees */
extern int16_t iatan2(int16_t y, int16_t x);

extern uint16_t isqrt(uint32_t a);
extern int16_t isin1024(uint8_t slivs);
extern int16_t icos1024(uint8_t slivs);
extern int8_t iasin(int16_t s);

#endif /* IMATHS_H */
