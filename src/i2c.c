/* (C) 2019 Harold Tay LGPLv3 */
#include <util/twi.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "i2c.h"

void i2c_init(void)
{
#define I2C_HZ 100000UL
  /*
    TWBR = (F_CPU/I2C_HZ - 16)/2
    with prescaler set to 1 (TWPS0 = TWPS1 = 0)
   */
  TWSR &= ~(_BV(TWPS0) | _BV(TWPS1)); /* /1 */
  TWBR = (F_CPU/I2C_HZ - 16)/2;
}

int8_t i2c(uint8_t flag)
{
  uint16_t i;
  TWCR = flag                         /* User command */
       | _BV(TWINT)                   /* Clear interrupt, start I2C */
       | _BV(TWEN)                    /* Enable TWI */
       ;
  for(i = 0; i < 12000; i++)          /* Busy-wait for completion */
    if (TWCR & _BV(TWINT)) return(0);
  return(I2C_TIMEOUT);
}

int8_t i2c_start(void)
{
  int8_t er;
  er = i2c(_BV(TWSTA) | _BV(TWEA));
  if (er) return(er);
  if( TW_STATUS == TW_START )return(0);
  if( TW_STATUS == TW_REP_START )return(0);
  return(I2C_NOSTART);
}

int8_t i2c_setreg(uint8_t addr, uint8_t reg, uint8_t val)
{
  int8_t er;
  er = i2c_start();
  if (er) return(er);
  if( i2c_out(addr) != TW_MT_SLA_ACK )return(I2C_NOSLACK);
  if( i2c_out(reg) != TW_MT_DATA_ACK )return(I2C_NODACK);
  if( i2c_out(val) != TW_MT_DATA_ACK )return(I2C_NODACK);
  i2c_stop();
  return(0);
}

int8_t i2c_read(uint8_t addr, uint8_t start, uint8_t * ary, uint8_t sz)
{
  uint8_t i;
  int8_t er;
  er = i2c_start();
  if (er) return(er);
  if( i2c_out(addr) != TW_MT_SLA_ACK )return(I2C_NOSLACK);
  if( i2c_out(start) != TW_MT_DATA_ACK )return(I2C_NODACK);
  if( i2c_start() )return(I2C_NORESTART);
  if( i2c_out(addr|1) != TW_MR_SLA_ACK )return(I2C_NOSLACK);
  for(i = 0; i < sz-1; i++){
    if( i2c_in_ak() )return(I2C_NODACK);
    ary[i] = TWDR;
  }
  if( i2c_in_nak() )return(I2C_NONACK);
  ary[sz-1] = TWDR;
  i2c_stop();
  return(0);
}
