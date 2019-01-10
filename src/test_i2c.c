/* (C) 2019 Harold Tay GPLv3 */
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/twi.h>
#include "tx.h"
#include "i2c.h"

int
main(void)
{
  int8_t er;
  int16_t ary[3];
  uint8_t i;

  tx_init();
  sei();

  _delay_ms(2000);

  i2c_init();

  tx_puts("i2c_start...\r\n");
  er = i2c_start();
  if (er) goto out;
  _delay_ms(100);
  tx_puts("i2c_out(60)...\r\n");
  er = i2c_out(60); /* compass */
  if (er != TW_MT_SLA_ACK) goto out;
  tx_puts("i2c_setreg(60, 0, 0x14)...\r\n");
  er = i2c_setreg(60, 0, 0x14);
  if (er) goto out;
  tx_puts("i2c_setreg(60, 1, 0x40)...\r\n");
  er = i2c_setreg(60, 1, 0x40);
  if (er) goto out;
  tx_puts("i2c_setreg(60, 2, 0x00)...\r\n");
  er = i2c_setreg(60, 2, 0x00);
  if (er) goto out;
  tx_puts("i2c_read(60, 0x03, ary, 6)...\r\n");
  er = i2c_read(60, 0x03, (void *)ary, 6);
  if (er) goto out;

  for(i = 0; i < 3; i++) {
    tx_putdec(ary[i]);
    tx_puts("\r\n");
  }

  tx_puts("Done.\r\n");

out:
  i2c_stop();

  tx_msg("error = ", er);
  for( ; ; );
}
