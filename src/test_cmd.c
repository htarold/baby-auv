/* (C) 2019 Harold Tay GPLv3 */
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "timer.h"
#include "tx.h"
#include "rx.h"
#include "mma.h"
#include "ppm_task.h"
#include "tude_task.h"
#include "syslog.h"
#include "task.h"
#include "ee.h"
#include "thrust.h"

/*
  Test machine command: make sure hardware works.
  New commands may be added.
 */

/*
  enable/disable front power and select input.
 */
void sel_rf(void)
{ PORTD |= _BV(PD6); PORTD &= ~_BV(PD7); }
void sel_gps(void)
{ PORTD |= _BV(PD7); PORTD &= ~_BV(PD6); }
void sel_cmd_voff(void)
{ PORTD |= _BV(PD7); PORTD |= _BV(PD6); }
void sel_cmd_von(void)
{ PORTD &= ~_BV(PD7); PORTD &= ~_BV(PD6); }
void sel_init(void)
{
  DDRD |= _BV(PD6);
  DDRB |= _BV(PD7);
  sel_cmd_von();
}

/*
  "GPS" (pass through) task
 */

uint8_t input_channel = 0;
int8_t passthru_task(void)
{
  static uint16_t count = 0;
  char ch;

  count++;

  if (1 == count) {
    /*
      Initialise: select GPS input (AB = 01); enable RX at 9600
     */
    DDRD |= _BV(PD6) | _BV(PD7);
    PORTD |= _BV(PD7);
    PORTB &= ~_BV(PD6);
    tx_puts("(passthru enabled)\r\n");
  } else if (500 == count) {          /* 5 seconds up */
    count = 0;
    PORTD &= ~_BV(PD7);               /* revert to command input */
    /* empty the cbuf */
    while (rx_havechar()) {
      ch = rx_getchar();
    }
    input_channel--;
    tx_puts("(passthru done)\r\n");
  } else {
    /* copy through all data in the cbuf */
    while (rx_havechar()) {
      ch = rx_getchar();
      tx_putc(ch);
    }
  }
  return(0);
}

/* directly manipulate config in mma.c */
extern struct servo servo;

void do_mma(char cmd)
{
  static uint16_t min, max;
  static uint16_t position;

  if (!position) position = ppm_get();

  switch (cmd) {
  case 'j': position += 6; break;
  case 'J': position += 30; break;
  case 'k': position -= 6; break;
  case 'K': position -= 30; break;
  case 'L': min = position; goto save;
  case 'H': max = position; goto save;
  case 's': mma_stop(); return;
  default: ;
  }
  ppm_set(position);
  position = ppm_get();
  return;

save:
  if (min > max) {
    servo.reversed = 1;
    servo.throw = ((min - max)/2)/6;
  } else {
    servo.reversed = 0;
    servo.throw = ((max - min)/2/6);
  }
  servo.centre = ((max + min)/2)/6;
  ee_store(&servo, &ee.servo, sizeof(servo));
  tx_puts("saved: mma servo settings:\r\n");
  tx_msg("servo.reversed = ", servo.reversed);
  tx_msg("servo.cntre = ", servo.centre);
  tx_msg("servo.throw = ", servo.throw);
}
static void help_mma(void)
{
  tx_puts("j/J/k/K: move in small/LARGE steps\r\n");
  tx_puts("L/H: mark limit at nose Low/High position\r\n");
  tx_puts("s: shut off MMA\r\n");
}

void help(void)
{
  help_mma();
  tx_puts("g:enable GPS pass through for 5 seconds.\r\n");
}

static int8_t thr_level = 0;
static int8_t thr_walk = 0;
static void thr(void)
{
  thrust_set(thr_level, thr_walk);
  tx_puts("thrust ");
  tx_putdec(thr_level);
  tx_puts(" walk ");
  tx_putdec(thr_walk);
  tx_puts("\r\n");
}
static void thr_inc(void)
{
  if (thr_level <= 90) thr_level += 10; thr();
}
static void thr_dec(void)
{
  if (thr_level >= -90) thr_level -= 10; thr();
}
static void thr_right(void) { thr_walk = 1; thr(); }
static void thr_left(void) { thr_walk = -1; thr(); }
static void thr_none(void) { thr_walk = 0; thr(); }

int8_t cmd_task(void)
{
  uint8_t r;

  if (1 == input_channel) {
    passthru_task();
    return(0);
  }

  while (rx_havechar()) {
    char ch;
    ch = rx_getchar();
    switch (ch) {
    /* servo commands: */
    case 'j': case 'J':
    case 'k': case 'K':
    case 'L': case 'H':
    case 's':  do_mma(ch); break;
    /* help */
    case '?': help(); break;
    /*
      Thruster commands
     */
    case '+': thr_inc(); break;
    case '-': thr_dec(); break;
    case '<': thr_left(); break;
    case '>': thr_right(); break;
    case '0': thr_none(); break;
    /*
      RF.  If RF is on, uart receiver not connected: commands
      cannot be read.
     */
    case 'r': sel_cmd_voff(); break;
    case 'R': sel_cmd_von(); break;
    /* debug */
    case '!':
      r = TCCR1A;
      tx_puts("TCCR1A=");
      tx_puts(fmt_x(r));
      tx_puts("\r\n");
      break;
    case 'g':  /* "gps" passthrough task */
      input_channel = 1;
      break;
    default:
      tx_puts("Unknown command <");
      tx_putc(ch);
      tx_puts(">\r\n");
      break;
    }
  }
  return(0);
}

/*
  Task to print out state vars
 */
int8_t print_task(void)
{
  static int8_t delay = 0;
  int8_t var, er;

  delay++;
  if (delay < 100) return(0);
  delay = 0;

  er = tude_get_pitch(&var);
  if (er) syslog_puts("tude_get_pitch() returned error\n");
  syslog_attr("theta", var);
  er = tude_get_roll(&var);
  if (er) syslog_puts("tude_get_roll() returned error\n");
  syslog_attr("phi", var);
  er = tude_get_heading(&var);
  if (er) syslog_puts("tude_get_heading() returned error\n");
  syslog_attr("hdg", var);
  return(0);
}

int8_t (*tasks[])(void) = {
  ppm_task,
  tude_task,
  cmd_task,
};

const char ident[] PROGMEM = __FILE__ " " __DATE__ " " __TIME__;

static void sync_tick(void)
{
  uint8_t huns;
  for(huns = time_uptimeh; huns == time_uptimeh; ) {
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_mode();
  }
}

static void bomb(void)
{
  syslog_puts("\nHalting\n");
  for( ; ; );
}

int
main(void)
{
  uint8_t i;
  int8_t er, reinit;

  tx_init();
  sei();

  for (i = 4; i > 0; i--) {
    tx_msg("Delay ", i);
    _delay_ms(1000);
  }

  time_init();
  syslog_init(ident, SYSLOG_DO_COPY_TO_SERIAL);
  rx_init();
  rx_enable(9600);
  sel_cmd_von();
  mma_init();
  thrust_init();

  er = tude_init();
  if (er) {
    syslog_puts("tude_init error ");
    syslog_i16d(er);
    syslog_putc('\n');
    bomb();
  }

  reinit = 0;

  for ( ; ; ) {
    /*
      We may skip a beat, but will always start at the top of a
      tick.
     */
    if (reinit) {
      for ( ; reinit > 0; reinit--)
        sync_tick();
      er = tude_init();
      if (er) {
        syslog_puts("tude_init reinit error ");
	syslog_i16d(er);
	syslog_putc('\n');
	bomb();
      }
      reinit = 0;
    }
    sync_tick();

    for (i = 0; i < sizeof(tasks)/sizeof(*tasks); i++) {
      er = tasks[i]();
      if (er) {
        syslog_puts("task ");
	syslog_i16d(i);
	syslog_puts(" returned ");
	syslog_i16d(er);
	syslog_putc('\n');
	if (ETASK_I2C == er) {
	  reinit = 1;
	  break;
	}
	bomb();
      }
    }
  }
}
