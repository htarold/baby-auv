#ifndef PWM_H
#define PWM_H
/*
  (C) 2019 Harold Tay LGPLv3
  Lower level that serves the thruster.
 */

#include <stdint.h>
#include "time.h"

#define PWMREV_BIT PB0
#define PWMREV_PORT PORTB
#define PWMREV_DDR DDRB

#define PWM_BIT PB1                   /* OC1A */
#define PWM_OCR OCR1A
#define PWM_PORT PORTB
#define PWM_DDR DDRB

extern void pwm_init(void);
extern void pwm_set(int8_t percent);
extern int8_t pwm_get(void);
#endif
