/*
  (C) 2019 Harold Tay LGPLv3
  I2C port expander
 */
#include "i2c.h"
#include "pcf8574.h"

#undef DEBUG
#ifdef DEBUG
#include "tx.h"
#include "fmt.h"
#define DBG(x) x
#else
#define DBG(x) /* nothing */
#endif

int8_t pcf8574_write(uint8_t addr, uint8_t data)
{
  int8_t er;
  er = i2c_start();
  if (er) return(er);
  DBG(tx_puts("pcf8574_write,addr="));
  DBG(tx_puthex(addr));
  DBG(tx_puts("\r\n"));
  DBG(tx_puts("pcf8574_write,data="));
  DBG(tx_puthex(data));
  DBG(tx_puts("\r\n"));
  er = i2c_out(addr|0);
  if (er != TW_MT_SLA_ACK) return(I2C_NOSLACK);
  er = i2c_out(data);
  if (er != TW_MT_DATA_ACK) return(I2C_NODACK);
  i2c_stop();
  return(0);
}
