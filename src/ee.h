/* (C) 2019 Harold Tay LGPLv3 */
#ifndef EE_H
#define EE_H
/*
  Anything to deal with EEPROM values are defined here, to
  ensure the same addresses are used across different binaries,
  e.g. a calibration binary will write, the operational binary
  will read, to/from the same address.

  DON'T define any EEPROM variables elsewhere than in this file
  and struct.

  Add new variables only to the end, to avoid changing earlier
  addresses.
 */
#include <avr/eeprom.h>
#include <stdint.h>

struct ee {
  struct tude {
    int16_t accel_offsets[3];
    int16_t cmpas_offsets[3];
    int16_t cmpas_scale[3];  /* multiply by this, to normalise */
  } tude;
  uint16_t odo_mm_per_rev;
  struct depth { uint16_t depth_gain1024; int16_t depth_offset; } depth;
  struct servo { int8_t reversed; uint8_t centre, throw; } servo;
  struct pitch { int8_t act1, act2; } pitch;
  struct twirl { int8_t offset; } twirl;
  struct odo { int16_t calib1024; } odo;
};

extern struct ee ee;

#define EE_MAYBE if ((TCNT1L & 0x3) == 0) 
uint8_t ee_load(void * p, void * ep, uint8_t size);
void ee_store(void * p, void * ep, uint8_t size);

#endif /* EE_H */
