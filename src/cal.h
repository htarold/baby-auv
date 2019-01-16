/* (C) 2019 Harold Tay LGPLv3 */
#ifndef CAL_H
#define CAL_H
#include <stdint.h>

/*
  Returns number of seconds since 2017.
 */
extern uint32_t cal_seconds(uint8_t dmy[3], uint8_t hms[3]);
extern void cal_task(void); /* for testing only */

#endif /* CAL_H */
