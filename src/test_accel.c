/* (C) 2019 Harold Tay GPLv3 */
/*
  Low level test for accel only.  LSM303DLHC.
 */

#include <util/delay.h>
#include "tx.h"
#include "fmt.h"
#include "i2c.h"

#if 1
static uint8_t accel_addr = (0x19<<1);  /* LSM303DLHC */
#else
static uint8_t accel_addr = (0x18<<1);  /* LSM303DLHC */
#endif

void bomb(int8_t e)
{
  tx_msg("i2c error ", e);
  for( ; ; );
}
void accelinit(void)
{
  int8_t e;
  i2c_init();
  if( (e = i2c_setreg(accel_addr, 0x20, 0x37)) ) bomb(e);
  /* set small/native endian (|0x40). */
  e = i2c_setreg(accel_addr, 0x23, 0x01);
  if (e) bomb(e);
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

  i = i2c_read(accel_addr, 0x2C, buf+4, 1);
  if (i) bomb(i);
  i = i2c_read(accel_addr, 0x2D, buf+5, 1);
  if (i) bomb(i);

#if 0
  for (i = 0; i < 6; i++) {
    tx_puthex(buf[i]); tx_putc(' ');
  }
#endif
  /* only for Z axis (easiest to manipulate) */
  /* binary(buf[4]); binary(buf[5]); tx_puts("\r\n"); */
  binary(buf[5]); binary(buf[4]); tx_puts("\r\n");
  val = buf[5] <<8 | buf[4];
  val /= 16;
  tx_msg("signed 16-bit: ", val);

  /*
  for (i = 0; i < 3; i++) {
    val = (uint16_t)((buf[(2*i)] <<8) | (buf[(2*i)+1] & 0xff));
    val <<= 2;
    tx_puts(fmt_u16d(val)); tx_puts("\r\n");
  }
   */
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
  accelinit();
  for ( ; ; ) {
    read6();
    _delay_ms(1000);
  }
}
