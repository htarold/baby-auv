/* (C) 2019 Harold Tay LGPLv3 */
#ifndef I2C_H
#define I2C_H
#define I2C_NOSTART -1
#define I2C_NOSLACK -2
#define I2C_NORESTART -3
#define I2C_NODACK -4
#define I2C_NONACK -5
#define I2C_TIMEOUT -6
#include <stdint.h>
#include <util/twi.h>

/*
  Addresses are the full 8-bit address (with LSB cleared to 0).
 */

extern void i2c_init(void);
extern int8_t i2c(uint8_t flag);
extern int8_t i2c_start(void);
extern int8_t i2c_setreg(uint8_t addr, uint8_t reg, uint8_t val);
extern int8_t i2c_read(uint8_t addr, uint8_t start, uint8_t * ary, uint8_t sz);
#define i2c_bh() /* nothing */
#define i2c_stop() TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO)
#define i2c_out(data) (TWDR = data, i2c(_BV(TWEA)), TW_STATUS)
#define i2c_in_ak() (i2c(_BV(TWEA)), TW_STATUS != TW_MR_DATA_ACK)
#define i2c_in_nak() (i2c(0), TW_STATUS != TW_MR_DATA_NACK)
#endif


