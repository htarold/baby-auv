#ifndef BAT_H
#define BAT_H
/*
  (C) 2019 Harold Tay LGPLv3
  Results in milliamps and millivolts.
  See bat.c for hardware config.
  These functions busy-wait for the ADC to complete (approx. 200us
  per conversion).
  No calibration is used.
 */
extern uint16_t bat_current(void);
extern uint16_t bat_voltage(void);
#endif /* BAT_H */
