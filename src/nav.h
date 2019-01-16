#ifndef NAV_H
#define NAV_H

/*
  (C) 2019 Harold Tay LGPLv3
  Easier to do artihmetic in angular measures where 2^26 angular
  measures constitutes a circle.  The UPPER 26 bits in a 32-bit
  int are used, so overflow is permissible and safe.
  Angular measures are also used for distance in (struct nav),
  which is the angle the distance subtends on the surface of the
  earth.
 */

typedef int32_t nav_t;
extern nav_t nav_make_nav_t(int32_t dmm);
extern int32_t nav_make_dmm(nav_t n);

#define NAV_INVALID_LAT ((INT32_MAX/2) + 1)
#define NAV_IS_NULL(n) ((n).lat == NAV_INVALID_LAT)
#define NAV_MAKE_NULL(n) \
do { (n).lat = NAV_INVALID_LAT; (n).lon = 0UL; } while (0)

struct nav_pt { nav_t lat, lon; };

/*
  For path planning.
  To convert angular measures to metres, multiply by 0.596.
 */
struct nav {
  unsigned long range:24;             /* in angular measures */
  int heading:8;
};

extern struct nav nav_rhumb(struct nav_pt * fr, struct nav_pt * to);
/* Undebugged
extern int8_t nav_reckon(struct nav_pt * position, int16_t north, int16_t east);
 */

#endif /* NAV_H */
