/* (C) 2019 Harold Tay GPLv3 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "tx.h"
#include "rx.h"
#include "time.h"
#include "thrust.h"
#include "handydefs.h"

/*
  Sample the encoder pin periodically.
  To get rpm, measure time (with coarse resolution of 20ms)
  for 1 rev.
 */
static uint16_t rpm;
static volatile uint16_t onn, off;
static void sample_encoder_pin(void)
{
  static uint8_t state, prev_state;
  static uint16_t counts, transitions;

  counts++;
  state = !!(RPM_PIN & _BV(RPM_BIT));

  if (state != prev_state) {
    transitions++;
    if (transitions == 8) {
      /*
        Calculate rpm.  8 transitions => 4 revolutions.
	period_ms = counts * 20ms / 4
	period_ms = counts * 5ms
	period_s = period_ms / 1000
	rpm = 60/period_s
	rpm = 60/(period_ms/1000)
	rpm = 60000/(period_ms)
	rpm = 60000/(counts * 5)
	rpm = 12000/(counts)
      */
      if (counts)
        rpm = 12000/counts;
      else
        rpm = -1;
      /* reset for next calculation */
      transitions = 0;
      counts = 0;   /* (re)start counting */
    }
    prev_state = state;
  }

  if (state)
    onn++;
  else
    off++;
}

int
main(void)
{
  int8_t thrust_level;
  int8_t thrust_walk;

  tx_init();
  sei();
  rx_init();
  rx_enable();
  time_init();
  thrust_init();

  thrust_walk = 0;
  thrust_level = 0;

  tx_puts("Delay 4\r\n"); _delay_ms(1000);
  tx_puts("Delay 3\r\n"); _delay_ms(1000);
  tx_puts("Delay 2\r\n"); _delay_ms(1000);
  tx_puts("Delay 1\r\n"); _delay_ms(1000);

  tx_puts("Starting\r\n");
  tx_puts("+:more fwd\r\n");
  tx_puts("-:more rev\r\n");
  tx_puts("R:walk right\r\n");
  tx_puts("L:walk left\r\n");
  tx_puts("0:Don't walk\r\n");

  time_register_fast(sample_encoder_pin);

  for( ; ; ){
    char ch;
    uint16_t local_rpm;

    if (!rx_havechar()) continue;
    ch = rx_getchar();
    switch (ch) {
    case '+': thrust_level += 10; break;
    case '-': thrust_level -= 10; break;
    case 'L': thrust_walk = -1; onn = off = 0; break;
    case 'R': thrust_walk = 1; onn = off = 0; break;
    case '0': thrust_walk = 0; onn = off = 0; break;
    default: tx_puts("What?\r\n"); break;
    }
    tx_putc(ch);
    _delay_ms(100);
    CONSTRAIN(thrust_level, -100, 100);
    thrust_set(thrust_level, thrust_walk);
    tx_msg("Thrust level: ", thrust_level);
    tx_msg("Thrust walk: ", thrust_walk);
    tx_msg("Time onn: ", onn);
    tx_msg("Time off: ", off);
    tx_msg("Revs: ", thrust_get_revs_x2()/2);
    cli();
    local_rpm = rpm;
    sei();
    tx_msg("RPM: ", local_rpm);
  }
}
