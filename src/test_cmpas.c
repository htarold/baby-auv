/* (C) 2019 Harold Tay GPLv3 */
/*
  Low level test for accel only.  LSM303DLHC.
 */

#include <util/delay.h>
#include "tx.h"
#include "fmt.h"
#include "i2c.h"

uint8_t cmpas_addr = (0x1E<<1);
uint8_t cmpas_xzy;

void bomb(int8_t e)
{
  tx_msg("i2c error ", e);
  for( ; ; );
}
void cmpasinit(void)
{
  int8_t e;
  uint8_t b;

  i2c_init();
  if( (e = i2c_setreg(cmpas_addr, 0x00, 0x14)) ) bomb(e);
  /* gain, 0x10 max gain, 0x70 min gain */
  if( (e = i2c_setreg(cmpas_addr, 0x01, 0x70)) ) bomb(e);
  if( (e = i2c_setreg(cmpas_addr, 0x02, 0x00)) ) bomb(e);
  if ((e = i2c_read(cmpas_addr, 0x0f, &b, 1))) bomb(e);
  cmpas_xzy = ('<' == b);
  tx_msg("cmpas_xzy = ", cmpas_xzy);
}

void binary(uint8_t b)
{
  int8_t i;
  for (i = 0; i < 8; i++) {
    if (b & (1<<7))
      tx_putc('1');
    else
      tx_putc('0');
    b <<= 1;
  }
}


void read6(void)
{
  int8_t i;
  uint8_t buf[6];
  int16_t val;

  i = i2c_read(cmpas_addr, 0x03, buf, 6);
  if (i) bomb(i);

  for (i = 0; i < 3; i++) {
    val = (uint16_t)((buf[(2*i)] <<8) | (buf[(2*i)+1] & 0xff));
    val <<= 2;
    tx_puts(fmt_i16d(val)); tx_puts(" ");
  }
  tx_puts("\r\n");
}

int
main(void)
{
  int8_t i;

  tx_init();
  for (i = 4; i > 0; i--) {
    tx_msg("Delay ", i);
    _delay_ms(1000);
  }

  i2c_init();
  cmpasinit();
  for ( ; ; ) {
    read6();
    _delay_ms(1000);
  }
}
