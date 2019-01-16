#ifndef HC12_H
#define HC12_H
#include <stdint.h>

/*
  (C) 2019 Harold Tay LGPLv3
  Basic transmission and reception using HC12.
 */

extern int8_t hc12_xmit_start(void);
extern int8_t hc12_recv_start(void);
extern void hc12_xmit_stop(void);
extern void hc12_recv_stop(void);
/* Not used: */
extern void hc12_xmit(uint8_t * buf, uint8_t size);
extern void hc12_tx_task(void);
extern void hc12_rx_task(void);
extern void hc12_setup_task(void); /* for testing */
#endif /* HC12_H */
