/* (C) 2019 Harold Tay GPLv3 */
/*
  Write contents of eeprom
 */

#include <avr/interrupt.h>
#include "ee.h"
#include "time.h"
#include "tx.h"

#define S(a) a, sizeof(*(a))
int
main(void)
{
  int8_t i;
  int8_t pitch_act1, pitch_act2;

  tx_init();
  time_init();
  sei();
  for(i = 0; i < 3; i++) {
    tx_msg("Starting ", 3 - i);
    time_delay(100);
  }

  /* these values seem to work quite well with the current setup */
  pitch_act1 = 8;
  pitch_act2 = 9;
  ee_store(&pitch_act1, &ee.pitch.act1, 1);
  ee_store(&pitch_act2, &ee.pitch.act2, 1);

  tx_puts("Done\r\n");
  for( ; ; );
}
