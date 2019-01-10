/* (C) 2019 Harold Tay LGPLv3 */
#include "sel.h"
#include <stdint.h>
#include "pcf8574.h"

#undef DEBUG
#ifdef DEBUG
#include "tx.h"
#include "fmt.h"
#define DBG(x) x
#else
#define DBG(x) /* nothing */
#endif

#define SEL_ADDR (PCF8574_ADDR | 0)   /* A0:2 pins tied to gnd */

uint8_t pcf8574_port;
int8_t sel_write(void)
{
  DBG(tx_puts("SEL_ADDR="));
  DBG(tx_puthex(SEL_ADDR));
  DBG(tx_puts("\r\n"));
  return(pcf8574_write(SEL_ADDR, pcf8574_port));
}
