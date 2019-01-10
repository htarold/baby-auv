#ifndef CTD_H
#define CTD_H
#include <stdint.h>
#include "ads1015.h"

#define CTD_ADDR ADS1015_ADDR_GND
/*
  (C) 2019 Harold Tay LGPLv3
  Read raw ADC counts.  Each call will block for few milliseconds if
  ADC has not already been set up (is the first call).  To set
  up, call with pointer null and call again in 10ms.

  CANNOT interlace calls:
  OK: ctd_get_rawc(0); ctd_get_rawc(&c); ctd_get_rawc(&c);...
  NOT OK: ctd_get_rawc(...); ctd_get_rawc(...); ctd_get_rawt(...);

  Actual ADC used is ADS1115, not 1015.

  Uses Timer 0; don't use it for anything else.

  XXX ADS1115 draws only 0.3mA when running.  Any way to keep it
  running all the time?  And scan through the channels?  No, no
  way to scan channels.

  XXX Single synchrnous call, but calls yield() internally.
  XXX Pointer address does not auto-increment.
 */

#define CTD_DEPTH 0
#define CTD_COND 1
#define CTD_THERM 2

/* Read calib values */
extern int8_t ctd_init(void);

/* Turn on CTD.  Draws quite a lot of power XXX. */
extern void ctd_start(void);

/* Call to specify which channel to read */
extern int8_t ctd_setup(uint8_t cfg);

/* After a delay, call to get converted value */
extern int8_t ctd_read(int16_t * p);

/* 1 -> is busy; 0 -> conversion complete; -1 -> error */
extern int8_t ctd_busy(void);

/* Turn off CTD */
extern void ctd_stop(void);

/* Depth is in cm */
#define ctd_setup_depth() ctd_setup(CTD_DEPTH)
extern int8_t ctd_get_depth(int16_t * dp);

/* Result in microsiemens; must multiply by cell constant. */
extern int8_t ctd_get_conductance(uint32_t * usp);

/*
  For testing, defined only if CTD_TASK is defined.
 */
extern void ctd_task(void);

/*
  Calibration is intended to be run under monotasking.
 */
extern void ctd_depth_calibrate(void);

extern int8_t ctd_calc_depth(int16_t raw, int16_t * dp);
extern int8_t ctd_calc_conductance(int16_t raw, uint32_t * gp);
extern int8_t ctd_calc_resistance(int16_t raw, uint16_t * rp);

extern int8_t ctd_get_resistance(uint16_t * rp);

/*
  All the rest for calibration use ONLY.
 */

struct ctd {
  int16_t depth_offset;
  uint16_t depth_gain1024;
  int16_t c_gnd;        /* Counts corresponding to ground level */
  int16_t c_vcc;        /* counts corresponding to vcc */
  uint16_t cond_r_1;    /* R1 is the upper resistor, around 1100 ohms */
  uint16_t cond_r_2;    /* R2 is the lower resistors, low value. */
  uint16_t cond_exc_hz; /* calibrated excitation frequency */
  uint16_t therm_r;     /* thermistor's divider resistance */
  /* XXX Other params ... */
};

extern int8_t ctd_store_eeprom(void);
extern int8_t ctd_load_eeprom(void);

#endif /* CTD_H */
