/* (C) 2019 Harold Tay GPLv3 */
/*
 */

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include "tx.h"
#include "rx.h"
#include "cbuf.h"
#include "time.h"
#include "handydefs.h"
#include "mma.h"
#include "ppm.h"
#include "ee.h"

static uint16_t min = 2000;
static uint16_t max = 600;
static uint8_t reverse = 0;
static uint8_t centre, throw;

static void print(void)
{
  tx_msg("Saved: servo.reversed = ", reverse);
  tx_msg("Saved: servo.centre/6 = ", centre);
  tx_msg("Saved: servo.throw/6 = ", throw);
}

static void save(void)
{
  if (min > max) {
    reverse = 1;
    throw = ((min - max)/2)/6;
  } else {
    reverse = 0;
    throw = ((max - min)/2)/6;
  }
  centre = ((min + max)/2)/6;
  eeprom_write_byte((uint8_t *)&ee.servo.reversed, reverse);
  eeprom_write_byte((uint8_t *)&ee.servo.centre, centre);
  eeprom_write_byte((uint8_t *)&ee.servo.throw, throw);
  print();
}

int
main(void)
{
  uint16_t position;
  int8_t i;

  tx_init();
  rx_init();
  rx_enable();
  sei();
  tx_puts("Starting\r\n");

  time_init();
  ppm_init();

  for(i = 0; i < 4; i++) {
    _delay_ms(1000);
    tx_msg("Delay ", 4 - i);
  }

  reverse = eeprom_read_byte((uint8_t*)&ee.servo.reversed);
  centre = eeprom_read_byte(&ee.servo.centre);
  throw = eeprom_read_byte(&ee.servo.throw);
  print();

  tx_puts("Stop servo output with s\r\n");
  tx_puts("Move mass with J/j/K/k\r\n");
  tx_puts("Mark nose High limit with H\r\n");
  tx_puts("Mark nose Low limit with L\r\n");

  position = 1350;
  for( ; ; ){
    char ch;
    if (!rx_havechar()) continue;
    ch = rx_getchar();
    switch (ch) {
    case 's': mma_stop(); continue;
    case 'J': position += 30; break;
    case 'j': position += 6; break;
    case 'K': position -= 30; break;
    case 'k': position -= 6; break;
/*
  mma convention:
  mma_set(-100) means mass moves to the rear, nose goes up.
  mma_set(100) means mass moves to forwards, nose goes down.
 */
    case 'H': min = position; save(); break;
    case 'L': max = position; save(); break;
    }
    CONSTRAIN(position, 600, 2000);
    ppm_set(position);
    tx_puts("position/6 = ");
    tx_putdec(position/6);
    tx_puts("    \r");
  }
}
