/*
  (C) 2019 Harold Tay LGPLv3
  Calibration values in builtin EEPROM.
 */
#include <avr/eeprom.h>
#include "ee.h"

struct ee ee EEMEM;

uint8_t ee_load(void * p, void * ep, uint8_t size)
{
  uint8_t i;
  eeprom_read_block(p, ep, size);
  for(i = 0; i < size; i++)
    if (-1 != ((int8_t *)(p))[i]) return(0);
  return(1);  /* all bytes unitialised */
}

void ee_store(void * p, void * ep, uint8_t size)
{
  eeprom_write_block(p, ep, size);
}
