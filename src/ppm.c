/* (C) 2019 Harold Tay LGPLv3 */
/*
  Works the servo (moving mass actuator).
  Compare register 1B is used.

  Servo needs a frame rate of 50Hz (20ms period) or longer, but
  tasks are called at 10ms intervals.  So we need to enable/disable
  the pulse every other time.  The task queue may be delayed by
  several ticks.  This means we need to actively enable the
  pulse, but we may passively let the pulse disable itself.

  This entails configuring the pin OC1B to:
  PWM mode (Fast PWM mode 14, done in timer.c);
  go high on overflow, low on match;

  The ISR is run on each match, and alternately enables/disables
  the pin.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include "time.h"
#include "ppm.h"
#include "handydefs.h"

#define PPM_LOW 645  /* microseconds.  Timer1 ticks every 1 us */
#define PPM_HIGH 1955

static uint16_t ppm_us;
uint16_t ppm_get(void) { return(ppm_us); }

static volatile uint8_t enable_output;
void ppm_stop(void) { enable_output = 0; }

void ppm_set(uint16_t us)
{
  if (us != 0)
    CONSTRAIN(us, PPM_LOW, PPM_HIGH);
  OCR1B = ppm_us = us;
  /* timer has been set up for fast PWM mode 14 */
  enable_output = 1;
}

ISR(TIMER1_COMPB_vect)
{
  static uint8_t alternately = 0;

  alternately = !alternately;
  if (alternately && enable_output) {
    /* Enable */
    TCCR1A |= _BV(COM1B1);
    TCCR1A &= ~(_BV(COM1B0));
  } else {
    /* Disable */
    TCCR1A &= ~(_BV(COM1B1));
    GPBIT_CLR(PPM);
  }
}

void ppm_init(void)
{
  GPBIT_CLR(PPM);
  GPBIT_OUTPUT(PPM);
  enable_output = 1;
  ppm_set((PPM_LOW + PPM_HIGH)/2);
  OCR1B = ppm_us;
  TIMSK1 |= _BV(OCIE1B);
}
