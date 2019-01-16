#ifndef THRUST_H
#define THRUST_H

#define RPM_BIT PD4                   /* Is PCINT20/PD4/Arduino D4*/
#define RPM_DDR DDRD
#define RPM_PORT PORTD
#define RPM_PIN PIND

/*
  (C) 2019 Harold Tay LGPLv3
  Interface to the thruster, including thruster walk functionality.
  pwm module contains macros for the pins.
 */

extern void thrust_init(void);
extern void thrust_reset_revs(void);
extern uint16_t thrust_get_revs_x2(void);
#define THRUST_WALK_TURN_RIGHT 1
#define THRUST_WALK_TURN_LEFT -1
#define THRUST_WALK_NONE 0
extern void thrust_set(int8_t percent, int8_t walk);
extern int8_t thrust_get_percent(void);
extern int8_t thrust_get_walk(void);

extern void thrust_start_rpm_count(void);
/* returns -1 if not done counting, which could mean 0 rpm. */
extern int16_t thrust_read_rpm_count(void);

/* For debugging */
#define EXTERN(x) /* nothing */
EXTERN(volatile int8_t start_100s)
EXTERN(volatile int8_t end_100s)
EXTERN(volatile int32_t start_uptime)
EXTERN(volatile int32_t end_uptime)
EXTERN(volatile int8_t nr_edges)
#endif /* THRUST_H */
