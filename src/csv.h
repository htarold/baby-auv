#ifndef CSV_H
#define CSV_H

#include <stdint.h>
#include "nav.h"

/*
  (C) 2019 Harold Tay LGPLv3
  Scans comma separated values in a buffer.
  Functions return number of chars consumed.
 */
/* Internal accumulator, sometimes used to return values */
extern int32_t csv_accum;

/* Scan at most nchars, converted value in accum. */
extern uint8_t csv_numeric(char * p, uint8_t nchars);

/* Not useful */
extern uint8_t csv_minutes(char * p);
extern uint8_t csv_degmins(char * p);

/* Scan a date or time field */
extern uint8_t csv_dateortime(char * p, uint8_t dmy[3]);

/*
  Scan 4 fields for lat and lon
  lat and lon are returned in units of minutes/10,000.
 */
extern uint8_t csv_latlon(char * p, struct nav_pt * np);

#endif /* CSV_H */
