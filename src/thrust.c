/* (C) 2019 Harold Tay LGPLv3 */
/*
  Thrusting and prop walk.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include "time.h"
#include "pwm.h"
#include "handydefs.h"
#include "thrust.h"

#define DBG_THRUST

#ifdef DBG_THRUST
#include "tx.h"
#define PRINT(a) a
#define DBG_LED_INIT DDRC |= _BV(PC3)
#define DBG_LED_ON   PORTC |= _BV(PC3)
#define DBG_LED_OFF  PORTC &= ~_BV(PC3)
#define DBG_LED_TOGGLE PINC |= _BV(PC3)
#else
#define PRINT(a) /* nothing */
#define DBG_LED_INIT /* nothing */
#define DBG_LED_ON   /* nothing */
#define DBG_LED_OFF  /* nothing */
#define DBG_LED_TOGGLE /* nothing */
#endif

static int8_t thrust_percent;
/* PCINT20 is PD4, the encoder pin */
#if 0
#define INT_ON SFR_SET(PCMSK2, PCINT20)
#define INT_OFF SFR_CLR(PCMSK2, PCINT20)
#else
#define INT_ON PCMSK2 = _BV(PCINT20)
#define INT_OFF PCMSK2 = 0
#endif

void thrust_init(void)
{
  thrust_percent = 0;
  pwm_init();
  GPBIT_INPUT(RPM);
  GPBIT_SET(RPM);                     /* pullup on */
  SFR_SET(PCICR, PCIE2);
  INT_ON;
  DBG_LED_INIT;
}

static volatile uint16_t thrust_revs;

void thrust_reset_revs(void)
{
  uint8_t sreg;
  sreg = SREG;
  cli();
  thrust_revs = 0;
  SREG = sreg;
}
uint16_t thrust_get_revs_x2(void)
{
  uint8_t sreg;
  uint16_t r;
  sreg = SREG;
  cli();
  r = thrust_revs;
  SREG = sreg;
  return(r);
}
static volatile int8_t thrust_walk;

static volatile uint8_t flag_count_rpm;
static void count_edges(void);

/*
  ISR called whenever pin changes.  The encoder goes high on
  light-to-dark transition, low on dark-to-light transition.
  XXX If using PCINT, could this ISR be triggered by a
  different pin?
 */
ISR(PCINT2_vect)
{
  int16_t modulation;
  static uint8_t prev_state;
  uint8_t is_rising;

  is_rising = !!(RPM_PIN & _BV(RPM_BIT));
  if (prev_state == is_rising)
    return;                           /* Not triggered for us */
  prev_state = is_rising;

  DBG_LED_TOGGLE;
  if (flag_count_rpm)
    count_edges();

  thrust_revs++;                      /* double counting */
  if (!thrust_walk) return;

  /*
    Assume Rising edge implies Rising prop blade;
    Fwd = !Rev;
    Rising = !Falling (edge and/or blade):
    Fwd   Rising   Walk R   Walk L
     1      0        -        +
     1      1        +        -
     0      1        -        +
     0      0        +        -
     (+/- means increase/decrease motor torque).
     Then (Fwd ^ Rising) & R => +

     XXX Don't reduce motor voltage too much, motor could stall.

   */

#define SLOW 30
  if (thrust_percent > 0) {           /* Forwards */
    if (!is_rising) {
      if (thrust_walk < 0) modulation = 100;
      else modulation = SLOW;
    } else {
      if (thrust_walk > 0) modulation = 100;
      else modulation = SLOW;
    }
  } else {                            /* Reverse XXX */
    if (is_rising) {
      if (thrust_walk < 0) modulation = -100;
      else modulation = -SLOW;
    } else {
      if (thrust_walk > 0) modulation = -100;
      else modulation = -SLOW;
    }
  }

  CONSTRAIN(modulation, -100, 100);
  pwm_set((int8_t)modulation);
}

void thrust_set(int8_t percent, int8_t walk)
{
  static int8_t old_percent = 0;

  CONSTRAIN(percent, -100, 100);
  CONSTRAIN(walk, -1, 1);
  thrust_percent = percent;
  thrust_walk = walk;

  if (0 == percent) {
    /* turn off interrupts, we might come to rest on an edge */
    INT_OFF;
  } else {
    INT_ON;
    /*
      If we are changing directions, or going to/from a stop,
      insert a delay to prevent high side mosfets from blowing up.
      XXX
    int16_t direction_change;
    direction_change = old_percent * percent;
    if (direction_change <= 0) time_delay(40);
    */
  }
  pwm_set(old_percent = thrust_percent);
}

int8_t thrust_get_percent(void)
{
  return(thrust_percent);
}
int8_t thrust_get_walk(void) { return(thrust_walk); }

#define MAX_EDGES 10                  /* must be an even number */
#define STATIC(x) static x;
STATIC(volatile int8_t start_100s)
STATIC(volatile int8_t end_100s)
STATIC(volatile int32_t start_uptime)
STATIC(volatile int32_t end_uptime)
STATIC(volatile int8_t nr_edges)
static void count_edges(void)
{
  if (0 == nr_edges) {
    start_100s = time_100s;
    start_uptime = time_uptime;
  } else {
    end_100s = time_100s;
    end_uptime = time_uptime;
  }
  if (MAX_EDGES == nr_edges) {
    flag_count_rpm = 0;               /* stop counting */
    return;
  }
  nr_edges++;
}

void thrust_start_rpm_count(void)
{
  uint8_t sreg;
  sreg = SREG;
  cli();
  start_100s = end_100s = 0;
  start_uptime = end_uptime = 0UL;
  /*
    Skip the first edge.  When thruster first starts, the
    encoder is turned on.  This can register as a transition.
    So start incrementing from -1.
   */
  nr_edges = -1;
  flag_count_rpm = 1;
  SREG = sreg;
}

int16_t thrust_read_rpm_count(void)
{
  int32_t hundredths;
  int16_t rpm;

  if (flag_count_rpm) return(-1);     /* Not yet ready */
  hundredths = (end_uptime - start_uptime);
  hundredths *= 100;
  hundredths += (end_100s - start_100s);
  /*
    RPM = (nr_edges/2)/(hundredths*0.01/60)
   */
  if (nr_edges < 0) return(0);
  rpm = 30*100*nr_edges/hundredths;
  if (nr_edges & 1 || nr_edges < MAX_EDGES)
    return(0 - rpm);                  /* inaccurate reading */
  return(rpm);
}
