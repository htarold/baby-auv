/*
  (C) 2019 Harold Tay LGPLv3
  serial (uart) output, can be interrupt driven.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>             /* for tx_puthex32 */
#include "tx.h"
#include "fmt.h"
#ifndef USART_BPS
#define USART_BPS 57600
#endif

#if 1
void tx_putc(char ch) { while( !(UCSR0A&_BV(UDRE0)) ); UDR0=ch; }

#else

#include "cbuf.h"
#include "handydefs.h"

cbuf_declare(tx_cbuf, 32); static struct tx_cbuf tx_cbuf = {0,};

ISR(USART_UDRE_vect)
{
  if (cbuf_haschar(tx_cbuf)) UDR0 = cbuf_get(tx_cbuf);
  else SFR_CLR(UCSR0B, UDRIE0);
}

void tx_putc(char ch)
{
  cbuf_put(tx_cbuf, ch);
  SFR_SET(UCSR0B, UDRIE0);
}
#endif

void tx_puts(char * s) { while( *s )tx_putc(*s++); }

void tx_init(void)
{
  UBRR0 = ((F_CPU/8)/USART_BPS) - 1;
  UCSR0B |= _BV(TXEN0);
  UCSR0A |= _BV(U2X0);
}

void tx_putdec(int16_t d) { tx_puts(fmt_i16d((int16_t)d)); }
void tx_putdec32(int32_t d) { tx_puts(fmt_i32d(d)); }
void tx_puthex(uint8_t x) { tx_puts(fmt_x(x)); }
void tx_puthex32(uint32_t x) { tx_puts(fmt_32x(x)); }
void tx_msg(char * s, int16_t d)
{
  tx_puts(s);
  tx_putdec(d);
  tx_puts("\r\n");
}
void tx_putpgms(const char * s)
{
  char ch;
  for ( ; ; s++) {
    ch = pgm_read_byte_near(s);
    if (!ch) break;
    tx_putc(ch);
  }
}
