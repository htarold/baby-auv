/* (C) 2019 Harold Tay LGPLv3 */
#ifndef ODO_H
#define ODO_H
#include "nav.h"

/* For debugging */
struct cartesian { int32_t north, east; };

/*
  Uses dead reckoning with current estimation, in addition to
  managing the propeller odometer.
 */

extern void odo_init(void);

/*
  Mark start of a leg.  The given position is saved for later
  use.
 */
extern int8_t odo_start(struct nav_pt * fix);

/*
  Call periodically so odo can integrate position.
  Period should be such that propeller has accumulated tens of
  revs at least.
 */
extern void odo_periodically(void);

/*
  Get an estimate of current position while underway, given the
  drift vector (which was previously returned by odo_stop()).
  odo_start() needs to have been called, and odo_stop() must not
  have been called.
 */
extern void odo_position(struct nav_pt * p, struct cartesian * drift);

/*
  With the given end point, using the start point provided to
  odo_start(), estimate drift due to current.  Odometer factor
  will adapt slightly.  Returns -1 on error, 0 on success, and 1
  if too soon to meaningfully compute drift.
 */
extern int8_t odo_stop(struct nav_pt * fix, struct cartesian * drift);

#endif /* ODO_H */
