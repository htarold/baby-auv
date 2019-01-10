/* (C) 2019 Harold Tay GPLv3 */
/*
  Calibrate the attitude sensor, placing the offset values in
  EEPROM.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "tude.h"
#include "tx.h"
#include "rx.h"

void halt(void)
{
  tx_puts("\r\nStopping.\r\n");
  for( ; ; );
}

int
main(void)
{
  int8_t er, i;

  tx_init();
  rx_init();
  rx_enable();
  sei();

  for(i = 4; i >= 0; i--) {
    tx_msg("Delay ", i);
    _delay_ms(1000);
  }

  er = accel_init();
  if (er < 0) {
    tx_msg("accel_init failed: ", er);
    halt();
  }

  if (er > 0)
    tx_puts("accel_init: not previously calibrated\r\n");
  else
    tx_puts("accel_init: recalibrating\r\n");

  er = cmpas_init();
  if (er < 0) {
    tx_msg("cmpas_init failed: ", er);
    halt();
  }

  if (er > 0)
    tx_puts("cmpas_init: not previously calibrated\r\n");
  else
    tx_puts("cmpas_init: recalibrating\r\n");

  tx_puts("accel_calib()...<enter> when flat and level.\r\n");
  while (!rx_havechar()) ;
  rx_getchar();

  er = accel_calib();
  if (er) {
    tx_msg("accel_calib() failed: ", er);
    halt();
  }
  tx_puts("accel_calib() done.\r\n");

  tx_puts("cmpas_calib()...<enter> to start.\r\n");
  while (!rx_havechar()) ;
  rx_getchar();

  er = cmpas_calib();
  if (er) {
    tx_msg("cmpas_calib() failed: ", er);
    halt();
  }
  tx_puts("cmpas_calib() done.\r\n");

  halt();
}
