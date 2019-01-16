/*
  (C) 2019 Harold Tay LGPLv3
  Soft uart port.  PD2(INT0) is used for RX, PD0 (which is the
  hardware uart's RX) is used for TX.  Hardware UART's TXD (PD1) pin
  is not touched.
  Speed is 9600.  Timer2 is used and cannot be used for any
  other purpose.

  When both spt_rx and spt_tx are enabled in a copy-through task,
  peak CPU is approx 38%.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include "cbuf.h"
#include "spt.h"

#undef PROFILE
#ifdef PROFILE
#define PIN_HI PORTC |= _BV(PC3);
#define PIN_LO PORTC &= ~_BV(PC3);
#else
#define PIN_HI /* nothing */
#define PIN_LO /* nothing */
#endif

struct spt_rx spt_rx = {0,};
struct spt_tx spt_tx = {0,};

#define TOP 103
#define BOTTOM 0

static uint8_t tccr2b;
void spt_set_speed_9600(void) { tccr2b = _BV(CS21); }
void spt_set_speed_2400(void) { tccr2b = _BV(CS21) | _BV(CS20); }

ISR(INT0_vect)
{
  uint8_t tmp;
  PIN_HI
  tmp = TCNT2 + TOP/2;                /* no overflow; TOP < MAX/2 */
  if (tmp >= TOP) tmp -= TOP;
  OCR2B = tmp;
  PIN_LO
}

static uint8_t spt_rx_bits = 0;
ISR(TIMER2_COMPB_vect)                /* ISR for rx, 2-7us at 8MHz */
{
  static uint8_t code;
  uint8_t b;

  PIN_HI
  b = READ_RX;
  if (0 == spt_rx_bits) {             /* Start bit */
    if (!b) spt_rx_bits++;            /* else, framing error */
    code = 0;
  } else if (spt_rx_bits < 9) {
    code >>= 1;
    if (b) code |= 0x80;
    spt_rx_bits++;
  } else {                            /* stop bit */
    if (b) cbuf_put(spt_rx, code);    /* else, framing error */
    code = spt_rx_bits = 0;
  }
  PIN_LO
}

static uint8_t spt_tx_bits = 0;
ISR(TIMER2_COMPA_vect)                /* ISR for tx */
{
  static uint8_t code;

  PIN_HI
  if (0 == spt_tx_bits) {
    if (cbuf_haschar(spt_tx)) {
      code = cbuf_get(spt_tx);
      TX_LO;                          /* Start bit (low) */
      spt_tx_bits++;
    }
  } else if (spt_tx_bits < 9) {
    if (code & 1) { TX_HI; }
    else { TX_LO; }
    code >>= 1;
    spt_tx_bits++;
  } else {                            /* stop bit (high) */
    TX_HI;
    spt_tx_bits = 0;
  }
  PIN_LO
}

#define ISRS_IN_USE (TIMSK2 & (_BV(OCIE2B)|_BV(OCIE2A)))
static void start_counter(void)
{
  if (ISRS_IN_USE) {
    TCCR2A = _BV(WGM21);              /* CTC mode 2 */
    TCCR2B = tccr2b;
    OCR2A = TOP;                      /* TOP value; ISR for tx */
    OCR2B = TOP/2;                    /* ISR for rx */
  }
}
static void stop_counter(void)
{
  if (ISRS_IN_USE) return;
  TCCR2B &= ~(_BV(CS22) | _BV(CS21) | _BV(CS20));
}

void spt_rx_start(void)
{
  if (!(EIMSK & _BV(INT0))){          /* is stopped */
    spt_rx_bits = 0;
    RX_SET_INPUT;
    TIMSK2 |= _BV(OCIE2B);
    OCR2B = TOP/2;
    EICRA |= _BV(ISC01);              /* falling edge */
    EICRA &= ~_BV(ISC00);             /* falling edge */
    EIMSK |= _BV(INT0);
    start_counter();
  }
}

void spt_rx_stop(void)
{
  EIMSK &= ~_BV(INT0);
  TIMSK2 &= ~_BV(OCIE2B);
  stop_counter();
}

void spt_tx_start(void)
{
  spt_tx_bits = 0;
  TX_SET_OUTPUT;
  TIMSK2 |= _BV(OCIE2A);
  start_counter();
}

void spt_tx_stop(void)
{
  TIMSK2 &= ~_BV(OCIE2A);
  stop_counter();
}

