/*
  (C) 2019 Harold Tay LGPLv3
  16-bit xorshift algorithm, after John Metcalf.
  It is claimed this has a period of 2^16 - 1 which is plenty
  for us.

  Ultimately this is a fixed series of numbers.  We will use
  this series of numbers to determine which blocks to encode in
  the fountain code.
 */

#include <stdint.h>
static uint16_t state = 1;
static uint16_t current = 0;

uint16_t prng_current(void) { return(current); }
uint16_t prng(void)
{
  state ^= state << 7;
  state ^= state >> 9;
  state ^= state << 8;
  return(state);
}
