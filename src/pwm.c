/*
  (C) 2019 Harold Tay LGPLv3
  Works the thruster.
  XXX Possible race condition in setting pwm_percent (despite lack of
  volatile): caller can get wrong value of pwm_percent, and
  throw us into reverse.

  So if the new thrust level is 0, then shut down the pwm.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include "time.h"  /* We are sharing timer resources */
#include "pwm.h"
#include "handydefs.h"

static volatile int8_t pwm_desired = 0;
static volatile int8_t pwm_percent = 0;

/*
  There is an issue when transitioning to/from 0 and negative.
  In order to get a smooth transition, we have to do 2 things
  simultaneously: set PWMREV; and change duty between 0 and 99%
  (or some large positive = small negative value).  We can't do
  this, so there is a hiccup where for 1 cycle it is fully
  negative.
 */


ISR (TIMER1_COMPA_vect)
{
  if (pwm_percent == pwm_desired) {
    SFR_CLR(TIMSK1, OCIE1A);
    return;
  }

  /* 0 to full speed in 2s, unless... */
  if (pwm_percent > 9 && pwm_desired > 9) pwm_percent = pwm_desired;
  else if (pwm_percent > pwm_desired) pwm_percent--;
  else if (pwm_percent < pwm_desired) pwm_percent++;

  /*
    If the pwm level is negative, the PWMREV bit is set.  In
    addition, depending on the hardware, the actual duty used
    could be the complement (100 - duty) or the negative (0 - duty).
   */
#define NEGATIVE_DUTY(duty) (0 - duty)

  if (pwm_percent < 0) {
    GPBIT_SET(PWMREV);
    OCR1A = NEGATIVE_DUTY(pwm_percent) * (TIMER_TOP/100);
  } else {
    GPBIT_CLR(PWMREV);
    OCR1A = pwm_percent * (TIMER_TOP/100);
  }
}

void pwm_set(int8_t desired)
{
  CONSTRAIN(desired, -100, 100);
  pwm_desired = desired;
  SFR_SET(TIMSK1, OCIE1A);
}

int8_t pwm_get(void) { return(pwm_percent); }

void pwm_init(void)
{
  GPBIT_OUTPUT(PWM);
  GPBIT_OUTPUT(PWMREV);
  GPBIT_CLR(PWMREV);

  /* Fast PWM mode 14 already set in timer.c */
  SFR_SET(TCCR1A, COM1A1);            /* non-inv pwm on OC1A = PB1 */
  SFR_CLR(TCCR1A, COM1A0);
  OCR1A = 0;
  pwm_set(0);
}
