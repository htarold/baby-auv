/* (C) 2019 Harold Tay LGPLv3 */
#ifndef MORSE_H
#define MORSE_H
#include <stdint.h>
#include "time.h"  /* piggy-backed on the ISR */

/*
  Using OC2B/PD3/Arduino 3 for the buzzer (timer 2).
 */
#define MORSE_BIT PD3
#define MORSE_DDR DDRD
#define MORSE_PORT PORTD
#define MORSE_PIN PIND

#undef MORSE_USE_TASK

/*
  Undefine if buzzer provides its own frequency and just needs
  to be turned on/off.  Pin is as defined in MORSE_*
  If defined, timer2 is used.
 */
#undef MORSE_NEEDS_TIMER

extern void morse_putc(char ch);
extern uint8_t morse_is_idle(void);
/* remove whatever's pending */
extern void morse_clear(void);
extern void morse_init(void);
extern void morse_10ms(void);

#endif /* MORSE_H */
