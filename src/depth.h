/* (C) 2019 Harold Tay LGPLv3 */
#ifndef DEPTH_H
#define DEPTH_H

/*
  This "thread" adjusts the pitch to maintain depth between dmax
  and dmin (centimetres).
 */
extern void depth_register(int16_t dmax, uint16_t dmin);
extern void depth_deregister(void);

#endif /* DEPTH_H */
