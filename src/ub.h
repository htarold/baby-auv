#ifndef UB_H
#define UB_H
#include <stdint.h>
#include "nav.h"

/*
  (C) 2019 Harold Tay LGPLv3
  Ublox (GPS) interface.

  GPGGA: 2=time; 3=lat; 4=N or S; 5=long; 6=E or W; 7=fix;
  8=nsats; 9=dilution; 10=altitude; 11=geoid; 12=?; 13=?;
  14=checksum
  GPRMC: 2=time; 3=A is active; 4=lat; 5=N/S; 6=lon; 7=E/W;
  8=speed; 9=track angle; 10=ddmmyy; 11=magnetic variation;
  12=E/W; 13=checksum

 */

extern int8_t ub_start(void); /* returns 0 if ok */
extern void ub_stop(void);

#define UB_OK         0  /* no error */
#define UB_ER_TMO    -2  /* timed out or input too long */
#define UB_ER_OVF    -3  /* token too big for buffer */
#define UB_ER_PARSE  -4  /* cannot parse field */
#define UB_ER_STALE  -5  /* stale position */
#define UB_ER_NODATA -6  /* no GPS data (yet) */
#define UB_ER_CKSUM  -7  /* NMEA checksum error */
#define UB_ER_NOFIX  -8  /* too few satellites for fix (but data
                            returned and may be usable) */
#define UB_ER_SEL    -9  /* selector error (probably I2C) */

/*
  ub_read_position() fetches the current position (in nav_t) and
  caches it.  ub_position() returns the cache, if still fresh.
  ub_read_position() will return 0 if successful, or a small
  positive number representing the DOP if available, or an
  error.
 */
extern int8_t ub_position(struct nav_pt * np);
extern int8_t ub_read_position(void);

/*
  If managed to synchronise local clock, *tp will contain the
  number of seconds since 2017 UTC.
  ub_read_datetime() will always fetch the time, even if the
  present value is not stale.
 */
extern int8_t ub_datetime(uint32_t * tp);
extern int8_t ub_read_datetime(void);

/*
  Returns <0 on error, or else the number of satellites in view.
 */
extern int8_t ub_read_nrsats(void);

#endif /* GP_H */
