/* (C) 2019 Harold Tay LGPLv3 */
#ifndef BG_H
#define BG_H

#include "tude.h"
#include <stdint.h>

#define BG_STACK_BYTES 160            /* our stack size */

/*
  Task that calls tude, bat, ctd modules, and operations that
  need to be done periodically and possibly concurrently.
 */

extern void bg_task(void) __attribute__ ((noreturn));
/* other tasks can use this too */
extern int8_t bg_attitude(void);

/*
  In effect a slow pitch controller.
  BG_PITCH_INVALID => disable controller
 */
#define BG_PITCH_INVALID 1025
void bg_pitch_set(int16_t pitch_sp, int16_t pitch_tol);
/*
  Actuate the mma directly (adds logging).
 */
void bg_mma(int8_t percent);

/*
  Other modules can check if a parameter has been updated.  It's
  a cheap pub/sub implementation.
 */

#define PUB_T_RESET(pub_t_var) pub_t_var = pub_updated
#define PUB_REFRESHED(which,pub_t_var) \
  (((pub_t_var&(1<<which))!=((pub_updated&(1<<which)))))

typedef uint8_t pub_t;
extern pub_t pub_updated;
extern struct angles pub_angles;      /* vehicle pose */
#define PUB_ANGLES 0
extern int16_t pub_mv;                /* battery voltage */
extern int16_t pub_ma;                /* current in milliamps */
#define PUB_BAT 1                     /* For mv and ma */
extern int16_t pub_cm;                /* depth in cm */
#define PUB_CM 2
extern uint32_t pub_us;               /* microsiemens */
#define PUB_US 3
extern uint16_t pub_degc;             /* thermistor */
#define PUB_DEGC 4
extern uint16_t pub_rpm;              /* thruster RPM, fwd only */
#define PUB_RPM 5
extern uint16_t pub_revs_x2;          /* thruster rev (rolls over) */
#define PUB_REVS_X2 6

#endif /* BG_H */
