#ifndef PCF8574_H
#define PCF8574_H
/*
  (C) 2019 Harold Tay LGPLv3
  I2C port expander, used in forward bulkhead RF board.
 */
#include <stdint.h>

/*
  OR the appropriate address pins that are tied to VCC
 */

#define PCF8574_A0 0x02
#define PCF8574_A1 0x04
#define PCF8574_A2 0x08
#define PCF8574_ADDR 0x40 /* XXX */

/* Write to port expander */
extern int8_t pcf8574_write(uint8_t addr, uint8_t data);

#endif /* PCF8574_H */
