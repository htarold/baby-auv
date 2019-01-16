/* (C) 2019 Harold Tay LGPLv3 */
#ifndef ADS1015_H
#define ADS1015_h
/*
  7-bit device address, depending on how the ADDR pin (pin 1) is
  connected.
  Most macros are the same for ADS1115 (16-bit version).
 */
/*
 */
#define ADS1015_ADDR_GND 0x48
#define ADS1015_ADDR_VDD 0x49
#define ADS1015_ADDR_SDA 0x4A
#define ADS1015_ADDR_SCL 0x4B

#define ADS1015_REG_CONV 0
#define ADS1015_REG_CONFIG 1
#define ADS1015_REG_THRSHLO 2
#define ADS1015_REG_THRSHHI 3

#define ADS1015_CFG_OS (1<<15)
#define ADS1015_MUX_AIN0AIN1 (0<<12)
#define ADS1015_MUX_AIN0AIN3 (1<<12)
#define ADS1015_MUX_AIN1AIN3 (2<<12)
#define ADS1015_MUX_AIN2AIN3 (3<<12)
#define ADS1015_MUX_AIN0GND  (4<<12)
#define ADS1015_MUX_AIN1GND  (5<<12)
#define ADS1015_MUX_AIN2GND  (6<<12)
#define ADS1015_MUX_AIN3GND  (7<<12)

#define ADS1015_PGA_6144MV  (0<<9)
#define ADS1015_PGA_4096MV  (1<<9)
#define ADS1015_PGA_2048MV  (2<<9)
#define ADS1015_PGA_1024MV  (3<<9)
#define ADS1015_PGA_512MV   (4<<9)
#define ADS1015_PGA_256MV   (5<<9)

#define ADS1015_MODE_1SHOT (1<<8)
#define ADS1015_MODE_CONTS (0<<8)

#define ADS1015_SPS_128 (0<<5)
#define ADS1015_SPS_250 (1<<5)
#define ADS1015_SPS_490 (2<<5)
#define ADS1015_SPS_920 (3<<5)
#define ADS1015_SPS_1K6 (4<<5)
#define ADS1015_SPS_2K4 (5<<5)
#define ADS1015_SPS_3K3 (7<<5)

#define ADS1115_SPS_008 (0<<5)
#define ADS1115_SPS_016 (1<<5)
#define ADS1115_SPS_032 (2<<5)
#define ADS1115_SPS_064 (3<<5)
#define ADS1115_SPS_128 (4<<5)
#define ADS1115_SPS_250 (5<<5)
#define ADS1115_SPS_475 (6<<5)
#define ADS1115_SPS_860 (7<<5)

#define ADS1015_COMP_REGULAR (0<<4)
#define ADS1015_COMP_WINDOW (1<<4)

#define ADS1015_COMP_ACTIVE_LOW (0<<3)
#define ADS1015_COMP_ACTIVE_HIGH (1<<3)

#define ADS1015_COMP_NOLATCH (0<<2)
#define ADS1015_COMP_LATCH (1<<2)

#define ADS1015_COMP_ASSERT1 (0)
#define ADS1015_COMP_ASSERT2 (1)
#define ADS1015_COMP_ASSERT3 (2)
#define ADS1015_COMP_NEVER   (3)

/* Set up the config register and begin conversion */
extern int8_t ads1015_setup(uint8_t addr7bit, uint16_t cfg);
/* extern int16_t ads1015_read(uint8_t addr7bit); deprecated */
/* Read the config register (maybe to check if conversion complete) */
extern int8_t ads1015_check(uint8_t addr7bit, uint16_t * cfgp);
extern int8_t ads1015_get(uint8_t addr7bit, int16_t * codep);

#endif /* ADS1015_H */
