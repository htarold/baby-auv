#ifndef PPM_H
#define PPM_H

/*
  (C) 2019 Harold Tay LGPLv3
  Pulse position modulation; interface to sevo used for pitch
  control.  Serves mma.{c,h}.
  MUST use PB2 = OC1B
 */
#define PPM_BIT  PB2                  /* OC1B */
#define PPM_PORT PORTB
#define PPM_DDR  DDRB

extern uint16_t ppm_get(void);
extern void ppm_set(uint16_t us);
extern void ppm_init(void);
extern void ppm_stop(void);
#endif /* PPM_H */
