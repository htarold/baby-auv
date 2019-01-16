#ifndef SPT_H
#define SPT_H
#include "cbuf.h"
/*
  (C) 2019 Harold Tay LGPLv3
  Software rx/tx.  Rx must be PD2 (to use INT0).
 */

#define TX_HI         PORTD |= _BV(PD0)
#define TX_LO         PORTD &= ~_BV(PD0)
/* PD0 is RXD, which we hijack as an output! */
#define TX_SET_OUTPUT \
do { DDRD |= _BV(PD0); UCSR0B &= ~_BV(RXEN0); } while (0)
#define READ_RX       (PIND & _BV(PD2))
#define RX_SET_INPUT  DDRD &= ~_BV(PD2)

cbuf_declare(spt_rx, 16); extern struct spt_rx spt_rx;
cbuf_declare(spt_tx, 16); extern struct spt_tx spt_tx;
void spt_set_speed_9600(void);
void spt_set_speed_2400(void);
void spt_rx_start(void);
void spt_rx_stop(void);
void spt_tx_start(void);
void spt_tx_stop(void);
#endif
