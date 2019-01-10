/* (C) 2019 Harold Tay LGPLv3 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "morse.h"
#include "time.h"
#include "cbuf.h"
#include "handydefs.h"

#define DBG_MORSE

#ifdef DBG_MORSE
#define PRINT(a) a
#else
#define PRINT(a) /* nothing */
#endif

/* How fast for signal, in units of 10ms */
#define INTER_DIT 4                   /* Duration of a di */
#define INTER_DAH (INTER_DIT*3)       /* Duration of a da */
#define INTER_DITDAH (INTER_DIT*2)    /* betw one di or da and next */
#define INTER_SPACE (INTER_DIT*7)     /* Duration between characters */

/*
  Use OC2B/PD3 for buzzer.
 */

/*
  Upper 3 bits encodes the number of dis or das.
  Lower 5 bits encodes the di/da patter, right justified, read
  right to left.  A clear bit is a di, a set bit is a da.
 */
#define LEN(len) (len<<5)

#define DIDA     (2 | LEN(2))  /* A */
#define DADIDIDI (1 | LEN(4))  /* B */
#define DADIDADI (5 | LEN(4))  /* C */
#define DADIDI   (1 | LEN(3))  /* D */
#define DI       (0 | LEN(1))  /* E */
#define DIDIDADI (4 | LEN(4))  /* F */
#define DADADI   (3 | LEN(3))  /* G */
#define DIDIDIDI (0 | LEN(4))  /* H */
#define DIDI     (0 | LEN(2))  /* I */
#define DIDADADA (14| LEN(4))  /* J */
#define DADIDA   (5 | LEN(3))  /* K */
#define DIDADIDI (2 | LEN(4))  /* L */
#define DADA     (3 | LEN(2))  /* M */
#define DADI     (1 | LEN(2))  /* N */
#define DADADA   (7 | LEN(3))  /* O */
#define DIDADADI (6 | LEN(4))  /* P */
#define DADADIDA (11| LEN(4))  /* Q */
#define DIDADI   (2 | LEN(3))  /* R */
#define DIDIDI   (0 | LEN(3))  /* S */
#define DA       (1 | LEN(1))  /* T */
#define DIDIDA   (4 | LEN(3))  /* U */
#define DIDIDIDA (8 | LEN(4))  /* V */
#define DIDADA   (6 | LEN(3))  /* W */
#define DADIDIDA (9 | LEN(4))  /* X */
#define DADIDADA (13| LEN(4))  /* Y */
#define DADADIDI (3 | LEN(4))  /* Z */
#define DADADADADA (31 | LEN(5)) /* 0 */
#define DIDADADADA (30 | LEN(5)) /* 1 */
#define DIDIDADADA (28 | LEN(5)) /* 2 */
#define DIDIDIDADA (24 | LEN(5)) /* 3 */
#define DIDIDIDIDA (16 | LEN(5)) /* 4 */
#define DIDIDIDIDI (0  | LEN(5)) /* 5 */
#define DADIDIDIDI (1  | LEN(5)) /* 6 */
#define DADADIDIDI (3  | LEN(5)) /* 7 */
#define DADADADIDI (7  | LEN(5)) /* 8 */
#define DADADADADI (15 | LEN(5)) /* 9 */

static const uint8_t PROGMEM morse_chars[36] = {
DIDA, DADIDIDI, DADIDADI, DADIDI, DI, DIDIDADI, DADADI, /* A-G */
DIDIDIDI, DIDI, DIDADADA, DADIDA, DIDADIDI, DADA, DADI, /* H-N */
DADADA, DIDADADI, DADADIDA, DIDADI, DIDIDI, DA, DIDIDA, /* M-U */
DIDIDIDA, DIDADA, DADIDIDA, DADIDADA, DADADIDI,         /* V-Z */
DADADADADA, DIDADADADA, DIDIDADADA, DIDIDIDADA, DIDIDIDIDA,
DIDIDIDIDI, DADIDIDIDI, DADADIDIDI, DADADADIDI, DADADADADI,
};

cbuf_declare(queue, 16); static struct queue queue;
void morse_clear(void)
{
  while (cbuf_haschar(queue))
    cbuf_get(queue);
}

void morse_putc(char ch)
{
  uint8_t mch, offset;
  if (ch >= 'a' && ch <= 'z')
    offset = (ch - 'a');
  else if (ch >= 'A' && ch <= 'Z')
    offset = (ch - 'A');
  else if( ch >= '0' && ch <= '9')
    offset = 26 + (ch - '0');
  else
    return;
  mch = pgm_read_byte(morse_chars + offset);
  {
    uint8_t sreg;
    sreg = SREG;
    cli();
    cbuf_put(queue, mch);
    SREG = sreg;
  }
}

/*
  Called to tun on or off the tone.
  100ms for a dit, 300 for dah, 500 for space.
 */

static uint8_t dida_len, dida_code, space_duration, tone_duration;

#define TONE_ON 1
#define TONE_OFF 0
#ifdef MORSE_NEEDS_TIMER
#define TONE_ENABLE SFR_SET(TCCR2A, COM2B1)
#define TONE_DISABLE SFR_CLR(TCCR2A, COM2B1)
#else
#define TONE_ENABLE GPBIT_SET(MORSE)
#define TONE_DISABLE GPBIT_CLR(MORSE)
#endif

/*
  Must be called every 10ms
 */
void morse_10ms(void)
{
  if (tone_duration) {                /* current di/da not finished */
    tone_duration--;
    if (!tone_duration) {             /* yes it's finished */
      TONE_DISABLE;                   /* start the space, assume not. */
      space_duration += INTER_DITDAH; /* .. end of char (short space) */
    }
  } else if (space_duration) {
    space_duration--;
  } else if (dida_len) {              /* keep processing current char */
    dida_len--;
    if (dida_code&1)
      tone_duration = INTER_DAH;
    else
      tone_duration = INTER_DIT;
    TONE_ENABLE;
    dida_code >>= 1;
    if (!dida_len)
      space_duration = INTER_SPACE - INTER_DITDAH;
  } else if (cbuf_haschar(queue)) {
    uint8_t bz;
    bz = cbuf_get(queue);
    dida_len = (bz>>5) & 7;
    dida_code = bz & 31;
  }
}

void morse_init(void)
{
  dida_len = dida_code = space_duration = tone_duration = 0;
  cbuf_initialise(queue);
  GPBIT_OUTPUT(MORSE);
  GPBIT_CLR(MORSE);
#ifdef MORSE_NEEDS_TIMER
  OCR2B = 128;
  TCCR2A = _BV(WGM21) | _BV(WGM20);   /* Fast PWM, TOP=0xff */
  TCCR2B = _BV(CS21) | _BV(CS20);     /* /32 */
  SFR_CLR(TCCR2A, COM2B0);
#else
  /*
    Buzzer generates its own frequency.
   */
#endif
}

uint8_t morse_is_idle(void)
{
  if (cbuf_nochar(queue) && !dida_len && !tone_duration
    && !space_duration)
    return(1);
  return(0);
  return(!!cbuf_nochar(queue));
}
