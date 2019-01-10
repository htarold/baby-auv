/* (C) 2019 Harold Tay LGPLv3 */
#ifndef CAL_H
#define CAL_H
#include <stdint.h>

extern uint32_t cal_seconds(uint8_t dmy[3], uint8_t hms[3]);
void cal_task(void); /* for testing only */

#endif /* CAL_H */
