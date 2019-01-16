/*
  (C) 2019 Harold Tay LGPLv3
  Transmit and receive using the HC12.
 */
#include <stdint.h>
#include "hc12.h"
#include "spt.h"
#include "sel.h"
#include "yield.h"

int8_t hc12_xmit_start(void)
{
  int8_t er;
  er = SEL_RF_ON;
  if (er) return(er);
  spt_set_speed_2400();
  spt_tx_start();
  return(0);
}
void hc12_xmit_stop(void) { spt_tx_stop(); }
int8_t hc12_recv_start(void)
{
  int8_t er;
  er = SEL_RF_ON;
  if (er) return(er);
  spt_set_speed_2400();
  spt_rx_start();
  return(0);
}
void hc12_recv_stop(void) { spt_rx_stop(); }
void hc12_xmit(uint8_t * buf, uint8_t size)
{
  while (size > 0) {
    while (cbuf_isfull(spt_tx)) yield();
    cbuf_put(spt_tx, *buf++);
    size--;
  }
}
