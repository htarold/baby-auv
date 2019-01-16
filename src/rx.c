/*
  (C) 2019 Harold Tay LGPLv3
  when enabled, ISR reads into circular buffer.  Dispenses from
  buffer byte-by-byte.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include "rx.h"
#include "cbuf.h"

struct rx_cbuf rx_cbuf;

ISR(USART_RX_vect)
{
  cbuf_put(rx_cbuf, UDR0);
  /*
    Possible to lap the buffer and lose data; ignore.
    At 9600bps, 8-byte buffer, lap in about 8ms.
   */
}

void rx_init(void) { }
void rx_enable(void)
{
  cbuf_initialise(rx_cbuf);
  UCSR0B |= _BV(RXCIE0) | _BV(RXEN0);
}
void rx_disable(void)
{
  UCSR0B &= ~(_BV(RXCIE0) | _BV(RXEN0));
}

int8_t rx_havechar(void) { return(cbuf_haschar(rx_cbuf)); }
char rx_getchar(void) { return(cbuf_get(rx_cbuf)); }
