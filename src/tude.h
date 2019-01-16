#ifndef TUDE_H
#define TUDE_H
#include <stdint.h>
/*
  (C) 2019 Harold Tay LGPLv3
  Higher level interface to lsm303.c without device specific
  dependencies.

  Which way is the unit installed?
  Assumes the chip is using a right-hand system.
  Avoids rotation matrices, since orientations are in full right
  angles.

  Orientation issue:
  Data output by chip assumes its own orientation.
  Angle calculations require NED, which may not match chip
  orientation.
  Also assumes chip is mounted flat, i.e. Z is either up or
  down, not any other orientation.

 */
#if 0  /* thruster board previous to 2018-05-06 */
#define AUVXFRONT(chipvec) -(chipvec)[0]
#define AUVYRIGHT(chipvec) -(chipvec)[1]
#define AUVZDOWN(chipvec)   (chipvec)[2]
#else  /* thruster board made on 2018-05-06 */
#define AUVXFRONT(chipvec)  (chipvec)[0]
#define AUVYRIGHT(chipvec) -(chipvec)[1]
#define AUVZDOWN(chipvec)  -(chipvec)[2]
#endif /* 0 */
/*
  For internal use:
 */
/* Calculates calibration based on raw vectors */
extern int8_t accel_calib(void);
extern int8_t accel_init(void); /* 0=ok; 1=not calibrated; <0:error */
/* Returns raw vector */
extern int8_t accel_read_raw(int16_t ary[3]);
/* Applies calibration, then reorients vector */
extern int8_t accel_read(int16_t ary[3]);
extern int8_t cmpas_calib(void);
extern int8_t cmpas_init(void);
extern int8_t cmpas_read_raw(int16_t ary[3]);
extern int8_t cmpas_read(int16_t ary[3]);

/*
  External interface:
 */

struct angles {
  int16_t sin_pitch;
  int8_t roll;
  int8_t heading;
};

/*
  Will read the IMU periodically (according to TUDE_RATE_*).
 */
extern void tude_task(void);

/*
  Put the current attitude in it, either reading anew, or using
  an older value if it is not stale (according to TUDE_RATE_*).
  And set the polling rate.
  0 is returned on success, non-zero is an error.
 */
#define TUDE_RATE_FAST 0
#define TUDE_RATE_SLOW 2
#define tude_get(ap, r) tude_read(ap)
extern int8_t tude_get(struct angles * tp, uint8_t rate);
extern int8_t tude_read(struct angles * ap);

#endif
