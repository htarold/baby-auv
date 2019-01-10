/* (C) 2019 Harold Tay LGPLv3 */
#ifndef NAV_H
#define NAV_H

/*
  it takes 26 bits to encode the angular measure.  Where latitudes
  and longitudes are concerned, the UPPER 26 bits are used, so
  that arithmetic causes overflow automatically.  Where
  distances (range field in struct nav) are concerned, the value is
  divided by 64 (right justified), so we won't normally need to use
  32-bit arithmetic.
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
extern int8_t nav_reckon(struct nav_pt * position, int16_t north, int16_t east);

#endif /* NAV_H */
