/* (C) 2019 Harold Tay GPLv3 */
#include <util/twi.h>
#include <util/delay.h>
#include "i2c.h"
#include "tx.h"
#include "ctd.h"  /* to start the conductivity clock */

static char * errstr(uint8_t e)
{
  switch (e) {
  case 0x08: return ("TW_START");
  case 0x10: return ("TW_REP_START");
  case 0x18: return ("TW_MT_SLA_ACK");
  case 0x20: return ("TW_MT_SLA_NACK");
  case 0x28: return ("TW_MT_DATA_ACK");
  case 0x30: return ("TW_MT_DATA_NACK");
  case 0x38: return ("TW_MT_ARB_LOST");
  case 0x40: return ("TW_MR_SLA_ACK");
  case 0x48: return ("TW_MR_SLA_NACK");
  case 0x50: return ("TW_MR_DATA_ACK");
  case 0x58: return ("TW_MR_DATA_NACK");
  case 0xA8: return ("TW_ST_SLA_ACK");
  case 0xB0: return ("TW_ST_ARB_LOST_SLA_ACK");
  case 0xB8: return ("TW_ST_DATA_ACK");
  case 0xC0: return ("TW_ST_DATA_NACK");
  case 0xC8: return ("TW_ST_LAST_DATA");
  case 0x60: return ("TW_SR_SLA_ACK");
  case 0x68: return ("TW_SR_ARB_LOST_SLA_ACK");
  case 0x70: return ("TW_SR_GCALL_ACK");
  case 0x78: return ("TW_SR_ARB_LOST_GCALL_ACK");
  case 0x80: return ("TW_SR_DATA_ACK");
  case 0x88: return ("TW_SR_DATA_NACK");
  case 0x90: return ("TW_SR_GCALL_DATA_ACK");
  case 0x98: return ("TW_SR_GCALL_DATA_NACK");
  case 0xA0: return ("TW_SR_STOP");
  case 0xF8: return ("TW_NO_INFO");
  case 0x00: return ("TW_BUS_ERROR");
  }
  if (e == 0) return("No error");
  if (e == I2C_NOSTART) return("I2C_NOSTART");
  if (e == I2C_NOSLACK) return("I2C_NOSLACK");
  if (e == I2C_NORESTART) return("I2C_NORESTART");
  if (e == I2C_NODACK) return("I2C_NODACK");
  if (e == I2C_NONACK) return("I2C_NONACK");
  tx_msg("Unknown value ", e);
  return("(unknown)");
}

int
main(void)
{
  uint8_t addr;
  int8_t a, b;
  int8_t i;

  for (i = 4; i > 0; i--) {
    tx_puts("\r\nDelay ");
    tx_putdec(i);
    _delay_ms(1000);
  }
  tx_puts("\r\nStarting\r\n");

  (void)ctd_start();
  for (addr = 0; addr < 127; addr++) {
    i2c_init();
    a = i2c_start();
    if (!a) {
      b = i2c_out((addr<<1));
    }
    i2c_stop();
    tx_puts("0x");
    tx_puthex(addr);
    tx_putc(':');
    if (!a) tx_puts(errstr(b));
    else tx_puts(errstr(a));
    tx_puts("\r\n");
  }

  for( ; ; );
}
