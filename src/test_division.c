/* (C) 2019 Harold Tay GPLv3 */
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include "time.h"
#include "tx.h"

#define TEST_PROFILE(type, arg) \
{ uint16_t s, e; type r; sleep_mode(); tx_puts(#arg); \
s = TCNT1; r = arg; e = TCNT1; \
tx_puts( " = "); tx_putdec(r); tx_msg(", elapsed = ", e - s); }

void divide32(uint32_t a, uint32_t b)
{
  TEST_PROFILE(uint32_t, a / b)
}
void divide16(uint16_t a, uint16_t b)
{
  TEST_PROFILE(uint16_t, a / b)
}

void div_16(int16_t a, int16_t b)
{
  uint16_t s, e;
  div_t r;
  sleep_mode();
  tx_puts("div_16"); \
  s = TCNT1;
  r = div(a, b);
  e = TCNT1;
  tx_puts( " = ");
  tx_putdec(r.quot);
  tx_puts( " + ");
  tx_putdec(r.rem);
  tx_puts( "/");
  tx_putdec(b);
  tx_msg(", elapsed = ", e - s);
}
void div_32(int32_t a, int32_t b)
{
  uint32_t s, e;
  ldiv_t r;
  sleep_mode();
  tx_puts("div_32"); \
  s = TCNT1;
  r = ldiv(a, b);
  e = TCNT1;
  tx_puts( " = ");
  tx_putdec32(r.quot);
  tx_puts( " + ");
  tx_putdec32(r.rem);
  tx_puts( "/");
  tx_putdec32(b);
  tx_msg(", elapsed = ", e - s);
}

int
main(void)
{
  tx_init();
  sei();
  time_init();

  /*
    Outputs:
a / b = 41, elapsed = 74
div_32 = 52 + 55/65, elapsed = 78
a / b = 52, elapsed = 26
div_16 = 52 + 55/65, elapsed = 28
   */
  divide32(23435,565);
  div_32(3435,65);
  divide16(3435,65);
  div_16(3435,65);

  for( ; ; );
}
