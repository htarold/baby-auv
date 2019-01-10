/* (C) 2019 Harold Tay LGPLv3 */
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include "time.h"
#include "handydefs.h"

volatile uint32_t time_uptime;        /* seconds */
volatile uint8_t time_100s;           /* hundredths of a second */

struct callback {
  void (*f[4])(void);
  uint8_t nr;
};
#define FAST callbacks[0]
#define SLOW callbacks[1]
static struct callback callbacks[2];  /* 0=fast, 1=slow */

static volatile uint8_t delay_count;
ISR(TIMER1_OVF_vect)
{
  int8_t i;
  /* Called every 10ms */
  time_100s++;
  if (delay_count) delay_count--;
  for(i = FAST.nr-1; i >= 0; i--) FAST.f[i]();
  if (time_100s >= 100) {
    time_100s = 0;
    time_uptime++;
    for(i = SLOW.nr-1; i >= 0; i--) SLOW.f[i]();
  }
}

void time_init(void)
{
  if (ICR1 == TIMER_TOP) return;      /* already initialised */
  ICR1 = TIMER_TOP;
  TCCR1A = _BV(WGM11);                /* Fast PWM mode 14 */
  TCCR1B = _BV(WGM13) | _BV(WGM12)    /* Fast PWM mode 14 */
         | _BV(CS11);                 /* /8 */
  TIMSK1 |= _BV(TOIE1);               /* enable overflow interrupt */
  FAST.nr = SLOW.nr = 0;
}

void time_delay(uint8_t huns)
{
  for(delay_count = huns / 2; delay_count > 0; ) {
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_mode();
  }
}

void time_register(uint8_t which, void (*f)(void))
{
  if (callbacks[which].nr >= ARRAY_COUNT(callbacks[0].f))
    return; /* XXX */
  callbacks[which].f[callbacks[which].nr++] = f;
}
void time_deregister(uint8_t which, void (*f)(void))
{
  uint8_t i;
  for(i = 0; i < callbacks[which].nr; i++)
    if (f == callbacks[which].f[i]) {
      callbacks[which].nr--;
      callbacks[which].f[i] = callbacks[which].f[callbacks[which].nr];
      break;
    }
}
