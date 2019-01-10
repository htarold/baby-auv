/* (C) 2019 Harold Tay GPLv3 */
/*
  For host computer.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "prng.c"

int
main(int argc, char ** argv)
{
  int i;
  uint16_t s;

  for (i = 0; i < 65536; i++) {
    s = prng();
    printf("%u %u\n", i, s);
  }
  exit(0);
}
