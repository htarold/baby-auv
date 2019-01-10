/* (C) 2019 Harold Tay LGPLv3 */
#ifndef RX_H
#define RX_H
#include <stdint.h>

/*
  when enabled, ISR reads into circular buffer.
  Speed is determined by tx.c (typically 57600).
 */
#include "cbuf.h"
cbuf_declare(rx_cbuf, 32); extern struct rx_cbuf rx_cbuf;
extern void rx_init(void);
extern void rx_enable(void);
extern void rx_disable(void);
extern int8_t rx_havechar(void);
extern char rx_getchar(void);
#define rx_bh() /* nothing */
#endif /* RX_H */
