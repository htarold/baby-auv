/* (C) 2019 Harold Tay LGPLv3 */
#ifndef YIELD_H
#define YIELD_H
/*
  To be included by all tasks (threads).  Also allows yield()
  to be redefined as a nop to allow standalone testing.
  yield() is defined in main.c.
 */
extern void yield(void);

/*
  Delay by calling yield this many times.
 */
extern void ydelay(uint8_t nr_yields);

/*
  Should contain some upper case chars.
 */
#include <avr/pgmspace.h>
#define panic(m,v) \
{ const static char s[] PROGMEM = m; panic_pgm(s, v); }

extern void panic_pgm(const char * msg, int16_t val)
  __attribute__ ((noreturn));

/* Mark location in stack, for debugging */
#define YIELD() { \
  int8_t sign; \
  __asm__ __volatile__ ( "ldi %0, 0xff" "\n\t" "push %0":"=a"(sign):); \
  yield(); \
  __asm__ __volatile__ ("pop %0":"=a"(sign):); \
}
#endif /* YIELD_H */
