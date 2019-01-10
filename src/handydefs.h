/* (C) 2019 Harold Tay LGPLv3 */
#ifndef HANDYDEFS_H
#define HANDYDEFS_H

#define GPBIT_CLR_(b) b ## _PORT &= ~_BV(b ## _BIT)
#define GPBIT_CLR(b) GPBIT_CLR_(b)

#define GPBIT_SET_(b) b ## _PORT |= _BV(b ## _BIT)
#define GPBIT_SET(b) GPBIT_SET_(b)

#define GPBIT_INPUT_(b) b ## _DDR &= ~_BV(b ## _BIT)
#define GPBIT_INPUT(b) GPBIT_INPUT_(b)

#define GPBIT_OUTPUT_(b) b ## _DDR |= _BV(b ## _BIT)
#define GPBIT_OUTPUT(b) GPBIT_OUTPUT_(b)

#define GPBIT_READ_(b) b ## _PIN & _BV(b ## _BIT)
#define GPBIT_READ(b) (GPBIT_READ_(b))

#define SFR_SET(reg,bit) reg |= _BV(bit)
#define SFR_CLR(reg,bit) reg &= ~(_BV(bit))

#define ARRAY_COUNT(a) (sizeof(a)/sizeof(*(a)))

#define CONSTRAIN(var,min,max) \
do { if (var>max) var = max; else if (var<min) var = min; } while (0)

#endif /* HANDYDEFS_H */
