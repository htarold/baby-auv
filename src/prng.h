/* (C) 2019 Harold Tay LGPLv3 */
#ifndef PRNG_H
#define PRNG_H
#include <stdint.h>
/* which ordinal number it is at currently */
extern uint16_t prng_current(void);
/* get a pseudo-random number */
extern uint16_t prng(void);
#endif /* PRNG_H */
