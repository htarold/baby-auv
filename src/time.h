#ifndef TIMER_H
#define TIMER_H
/*
  (C) 2019 Harold Tay LGPLv3
  Keeps processor uptime.
  Timer1 with prescaler of /8, with this TOP value gives 20ms
  before overflow.  This 20ms is used to update the uptime
  clock, and is also the PPM frame rate.

  Each tick is 1us (8MHz)or 0.5us (16MHz).
 */
#if F_CPU == 8000000UL
#define TIMER_TOP 9999
#elif F_CPU == 16000000UL
#define TIMER_TOP 19999
#else
#error Expected 8MHz or 16MHz clock only.
#endif
/*
  Prescaler divides by 8.
 */
#define TIMER_TICK_MS (((TIMER_TOP+1)*1000L)/(F_CPU/8))

/*
  Uptime clock in seconds, and hundredths (20ms resolution).
 */
extern volatile uint32_t time_uptime; /* seconds */
#define time_lsb *((uint8_t *)&time_uptime)
extern volatile uint8_t time_100s;    /* hundredths of a second */
#define time_uptimeh time_100s        /* compatibility */
extern void time_init(void);
extern void time_delay(uint8_t huns); /* calls sleep_mode() */
/* void (*f)(void) */
/* Deprecated functions: */
extern void time_register(uint8_t which, void (*f)(void));
extern void time_deregister(uint8_t which, void (*f)(void));
#define time_register_fast(f) time_register(0, f)
#define time_deregister_fast(f) time_deregister(0, f)
#define time_register_slow(f) time_register(1, f)
#define time_deregister_slow(f) time_deregister(1, f)

extern void time_task(void);

#endif /* TIMER_H */
