/* (C) 2019 Harold Tay LGPLv3 */
/*
  Measuring the execution time of various functions/operation.
 */
#include "tx.h"
#include "imaths.h"

#define PRESCALAR 1
#define NS_PER_TICK ((PRESCALAR*1000UL)/(F_CPU/1000000UL))
#define MEASURE(call) \
{ int32_t t; tx_puts(#call " -> "); TCNT1 = 0; call; \
t = TCNT1; tx_putdec32(t*NS_PER_TICK); tx_puts("ns\r\n"); }

void use(uint32_t unused)
{
  tx_puts("\r\n\t");
  tx_putdec32(unused);
  tx_puts("\r\n");
}

void div16(int16_t a, int16_t b)
{
  int16_t c;
  MEASURE(c = a / b);
  use(c);
}
void mult32(int32_t a, int32_t b)
{
  int32_t c;
  MEASURE(c = a * b);
  use(c);
}
void div32(int32_t a, int32_t b)
{
  int32_t c;
  MEASURE(c = a / b);
  use(c);
}
void add32(int32_t a, int32_t b)
{
  int32_t c;
  MEASURE(c = a + b);
  use(c);
}
void sub32(int32_t a, int32_t b)
{
  int32_t c;
  MEASURE(c = a - b);
  use(c);
}
void floatops(float a, float b)
{
  float c;
  MEASURE(c = (float)a * b);
  use(c);
  MEASURE(c = (float)a / b);
  use(c);
}
void longlongops(int32_t a, int32_t b)
{
  int64_t tmp64;
  MEASURE(tmp64 = (int64_t)a * (int64_t)b);
  use(tmp64);
}

int
main(void)
{
  int32_t a32, b32, c32;
  int8_t a8, b8, c8;
  int16_t a16, b16;

  ICR1 = 65535;
  TCCR1A = _BV(WGM11);                /* Fast PWM mode 14 */
  TCCR1B = _BV(WGM13) | _BV(WGM12)    /* Fast PWM mode 14 */
         | _BV(CS10);                 /* /1 */

  tx_init();
  /* Leave interrupts off */

  a8 = 1;
  b8 = 2;
  MEASURE(c8 = a8 + b8);
  use(c8);
  MEASURE(iatan2(504, 1192));
  MEASURE(isqrt(1192));
  MEASURE(isin1024(97));
  MEASURE(icos1024(98));
  MEASURE(iasin(906));

  a32 = 1024L*1024L + 234;
  b32 = 98163;
  div16(a32, b32);
  div32(a32, b32);
  mult32(a32, b32);
  add32(a32, b32);
  sub32(a32, b32);
  floatops((float)a32, (float)b32);
  longlongops(a32, b32);


  for( ; ; );
}
