/* (C) 2019 Harold Tay LGPLv3 */
#ifndef GP_H
#define GP_H
#include <stdint.h>

extern uint8_t gps_lat(char * p, int32_t * result);
extern uint8_t gps_lon(char * p, int32_t * result);
extern uint8_t gps_time(char * p, uint8_t hmsh[4]);
extern uint8_t gps_date(char * p, uint8_t dmy[3]);

#endif /* GP_H */
