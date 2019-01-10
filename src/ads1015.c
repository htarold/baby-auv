/* (C) 2019 Harold Tay LGPLv3 */

#include <stdint.h>
#include "ads1015.h"
#include "i2c.h"

int8_t ads1015_setup(uint8_t addr7bit, uint16_t cfg)
{
  int8_t er, res;

  res = -1;
  er = i2c_start();
  if (er) goto bomb;
  res--;
  er = i2c_out((addr7bit<<1)|0);      /* write */
  if (er != TW_MT_SLA_ACK) goto bomb;
  er = i2c_out(ADS1015_REG_CONFIG);
  res--;
  if (er != TW_MT_DATA_ACK) goto bomb;
  er = i2c_out((cfg>>8)&0xff);
  res--;
  if (er != TW_MT_DATA_ACK) goto bomb;
  er = i2c_out(cfg&0xff);
  res--;
  if (er != TW_MT_DATA_ACK) goto bomb;
  res = 0;
bomb:
  i2c_stop();
  return(res);
}
#if 0
/* Deprecated interface */
int16_t ads1015_read(uint8_t addr7bit)
{
  uint16_t code;
  int8_t res, er;

  code = 0;
  res = -1;
  er = i2c_start();
  if (er) goto bomb;
  er = i2c_out((addr7bit<<1)|0);      /* write */
  res--;
  if (er != TW_MT_SLA_ACK) goto bomb;
  er = i2c_out(ADS1015_REG_CONV);
  res--;
  if (er != TW_MT_DATA_ACK) goto bomb;
  i2c_stop();
  er = i2c_start();
  res--;
  if (er) goto bomb;
  er = i2c_out((addr7bit<<1)|1);      /* read */
  res--;
  if (er != TW_MR_SLA_ACK) goto bomb;
  er = i2c_in_ak();
  res--;
  if (er) goto bomb;
  code = TWDR<<8;
  er = i2c_in_nak();
  res--;
  if (er) goto bomb;
  code |= TWDR;
bomb:
  i2c_stop();
  return(er?res:code);
}
#endif

static int8_t read_register(uint8_t addr7bit, uint8_t reg, uint16_t * regp)
{
  uint16_t code;
  int8_t res, er;

  code = 0;
  res = -1;
  er = i2c_start();
  if (er) goto bomb;
  er = i2c_out((addr7bit<<1)|0);      /* write */
  res--;
  if (er != TW_MT_SLA_ACK) goto bomb;
  er = i2c_out(reg);
  res--;
  if (er != TW_MT_DATA_ACK) goto bomb;
  i2c_stop();
  er = i2c_start();
  res--;
  if (er) goto bomb;
  er = i2c_out((addr7bit<<1)|1);      /* read */
  res--;
  if (er != TW_MR_SLA_ACK) goto bomb;
  er = i2c_in_ak();
  res--;
  if (er) goto bomb;
  code = TWDR<<8;
  er = i2c_in_nak();
  res--;
  if (er) goto bomb;
  code |= TWDR;
  *regp = code;
bomb:
  i2c_stop();
  return(er?res:0);
}

int8_t ads1015_get(uint8_t addr7bit, int16_t * codep)
{
  return read_register(addr7bit, ADS1015_REG_CONV, (uint16_t *)codep);
}

/* Returns the 16-bit config register */
int8_t ads1015_check(uint8_t addr7bit, uint16_t * cfgp)
{
  return read_register(addr7bit, ADS1015_REG_CONFIG, cfgp);
}
