/* (C) 2019 Harold Tay LGPLv3 */
/*
  Simple interface to i2c lcd backpack.
  Assumes 1602 size.
 */

#include "i2c.h"
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "lcd1602.h"

/*
  How the port expander defines its bits.
 */
#define RS 0
#define RW 1
#define EN 2
#define BL 3                          /* Back light */
#define D4 4
#define D5 5
#define D6 6
#define D7 7

#define PULSE(x) pulse(x)

static uint8_t backlight_on;
void lcd_backlight(uint8_t on) { backlight_on = !!on; }

static int8_t send(uint8_t d)
{
  int8_t er;
  er = i2c_start();
  if (er) return(er);
  er = i2c_out(LCD1602_WRADDR);
  if (er != TW_MT_SLA_ACK) return(I2C_NOSLACK);
  er = i2c_out(d);
  if (er != TW_MT_DATA_ACK) return(I2C_NODACK);
  i2c_stop();
  return(0);
}

/* Upper nybble of d is sent */
static int8_t pulse(uint8_t d)
{
  int8_t er;
  d |=  _BV(BL);
  if (backlight_on) d |= _BV(BL);
  else d &= ~_BV(BL);
  d |=  _BV(EN); er = send(d);
  if (er) return(er);
  _delay_us(1);
  d &= ~_BV(EN); er = send(d);
  return(er);
}

static uint8_t column;

int8_t lcd_init(void)
{
  static const uint8_t cmds[] PROGMEM = {
    _BV(D5) | _BV(D4),                /* 8-bit mode */
    _BV(D5),                          /* 4-bit mode */
    _BV(D5), _BV(D7),                 /* Display has 2 lines */
    0, _BV(D7),                       /* Display off */
    0, _BV(D6) | _BV(D5),             /* I/D, S */
    0, _BV(D7) | _BV(D6) | _BV(D5),   /* Display on */
    0, _BV(D6) | _BV(D5),             /* Entry mode set */
  };
  int8_t er, i;

  _delay_ms(40);
  er = PULSE(_BV(D5) | _BV(D4));
  if (er) return(er);
  _delay_ms(5);
  er = PULSE(_BV(D5) | _BV(D4));
  if (er) return(er);
  _delay_us(100);
  er = PULSE(_BV(D5) | _BV(D4));
  if (er) return(er);
  for (i = 0; i < sizeof(cmds); i++) {
    uint8_t d;
    d = pgm_read_byte_near(cmds + i);
    er = PULSE(d);
    if (er) return(er);
  }
  column = 0;
  return(0);
}

int8_t lcd_put(uint8_t ch)
{
  int8_t er;

  er = PULSE(_BV(RS) | (ch & 0xf0));
  if (er) return(er);
  er = PULSE(_BV(RS) | (ch <<4));
  if (er) return(er);
  column++;
  return(0);
}

/* Clears to end of current line, then goes to start of
requested line (could be same line, 0 or 1) */

int8_t lcd_clrtoeol(void)
{
  int8_t er;
  while (column < 16) {
    er = lcd_put(' ');
    if (er) return(-1);
  }
  return(0);
}

int8_t lcd_goto(uint8_t line, uint8_t col)
{
  uint8_t d;
  int8_t er;

  d = _BV(D7);
  if (line) d |= _BV(D6);
  column &= (1<<6)-1;
  d |= col;
  er = PULSE(d&0xf0);
  if (er) return(er);
  er = PULSE(d<<4);
  if (er) return(er);
  column = col;
  return(er);
}
