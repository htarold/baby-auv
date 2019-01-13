#ifndef MMA_H
#define MMA_H
#include <stdint.h>
#include "ppm.h"
/*
  (C) 2019 Harold Tay LGPLv3
  Moving mass actuator.
  mma_set(-100) means mass moves to the rear, nose goes up.
  mma_set(100) means mass moves to forwards, nose goes down.
 */

extern void mma_init(void);
extern int8_t mma_get(void);
/* (mass moves to rear) -100 <= pos <= 100 (mass moves fwd) */
extern void mma_set(int8_t pos);

/* Returns -1 if limit exceeded, sets actuator to limit. */
extern int8_t mma_inc(int8_t delta);
#define mma_stop() ppm_stop()

#endif /* MMA_H */
