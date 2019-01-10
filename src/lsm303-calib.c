/* (C) 2019 Harold Tay LGPLv3 */

/*
  LSM303
  Accel gives 12 bit values, but range is +/-2g, so 1g
  corresponds to 1024.
  Bits are LEFT aligned.

  For orientation, follow Table 2 of LSM app note:
               Ax Ay Az
  Z down       0  0  +1g
  Z up         0  0  -1g
  Y down       0 +1g 0
  Y up         0 -1g 0
  X down      +1g 0  0
  X up        -1g 0  0
  These are the axes of the AUV, XYZ are NED.  Change the
  AUVXFRONT/AUVYLEFT/AUVZUP macros in tude.h until the a[]
  values have the correct sign.

  Compass REQUIRES calibration; Z axis is generally badly off.
  Compass gain should also not be max because the values may
  overflow before applying the calibration offsets.
 */

#include <avr/eeprom.h>
#include "i2c.h"
#include "tude.h"
#include "tx.h"
#include "fmt.h"
#include "ee.h"
#include "morse.h"   /* for definition of the buzzer pin */

#define DBG_LSM303
#ifdef DBG_LSM303
#define PRINT(a) a
#else
#define PRINT(a) /* nothing */
#endif

static struct tude calib;

static void check_overflow(int16_t ary[3])
{
  int8_t i, ovf;
  for (ovf = i = 0; i < 3; i++) {
    if (ary[i] >= INT8_MAX || ary[i] <= INT8_MIN) {
      tx_puts("Calibration overflow, subscript ");
      tx_putdec(i);
      tx_msg(": ", ary[i]);
      ovf = 1;
    }
  }
  while (1 == ovf) ;
}

/* MUST be placed level */
int8_t accel_calib(void)
{
  int8_t e, i;
  int16_t ary[3];
  int16_t sum[3];
  sum[0] = sum[1] = sum[2] = 0;
  for(i = 0; i < 8; i++){
    if( (e = accel_read_raw(ary)) )return(e);
    sum[0] += ary[0];
    sum[1] += ary[1];
    sum[2] += ary[2];
  }
  sum[0] /= 8;
  sum[1] /= 8;
  sum[2] /= 8;
  check_overflow(sum);
  /*
    XXX Need to account for chip orientation.
    value + calib = 0 or 1024 or -1024
   */
  for (i = 0; i < 3; i++) {
    /* around 0? */
    if (sum[i] > -150 && sum[i] < 150)
      calib.accel_offsets[i] = 0 - sum[i];
    else if (sum[i] > 896 && sum[i] < 1152)
      calib.accel_offsets[i] = 1024 - sum[i];
    else if (sum[i] < -896 && sum[i] > -1152)
      calib.accel_offsets[i] = sum[i] - 1024;
    else
      return(-1);
  }
  tx_msg("## sum[0] = ", sum[0]);
  tx_msg("## sum[1] = ", sum[1]);
  tx_msg("## sum[2] = ", sum[2]);
  tx_msg("## calib.accel_offsets[0] = ", calib.accel_offsets[0]);
  tx_msg("## calib.accel_offsets[1] = ", calib.accel_offsets[1]);
  tx_msg("## calib.accel_offsets[2] = ", calib.accel_offsets[2]);
  ee_store(calib.accel_offsets, ee.tude.accel_offsets,
    sizeof(ee.tude.accel_offsets));
  return(0);
}

struct axis {
#define ARYSZ 8
  int16_t max[ARYSZ];
  int16_t min[ARYSZ];
};
static inline uint8_t min(int16_t ary[ARYSZ])
{
  uint8_t pos, i;
  pos = 0;
  for(i = 1; i < ARYSZ; i++)
    if( ary[i] < ary[pos] )pos = i;
  return(pos);
}
static inline uint8_t max(int16_t ary[ARYSZ])
{
  uint8_t pos, i;
  pos = 0;
  for(i = 1; i < ARYSZ; i++)
    if( ary[i] > ary[pos] )pos = i;
  return(pos);
}
static inline uint8_t savepoints(int16_t val, struct axis * ap)
{
  uint8_t pos, saved;
  saved = 0;
  pos = min(ap->max);
  if( val > ap->max[pos] ){
    tx_puts("max:");
    tx_putdec(ap->max[pos]); tx_puts("<-"); tx_putdec(val);
    tx_puts("\r\n");
    ap->max[pos] = val;
    saved = 1;
  }
  pos = max(ap->min);
  if( val < ap->min[pos] ){
    tx_puts("min:");
    tx_putdec(ap->min[pos]); tx_puts("<-"); tx_putdec(val);
    tx_puts("\r\n");
    ap->min[pos] = val;
    saved = 1;
  }
  return(saved);
}

int8_t cmpas_calib(void)
{
  static struct axis axes[3];
  uint8_t ax;
  int8_t e, i;
  uint16_t saved[3];

  for(ax = 0; ax < 3; ax++){
    saved[ax] = 0;
    for(i = 0; i < ARYSZ; i++){
      axes[ax].max[i] = INT16_MIN;
      axes[ax].min[i] = INT16_MAX;
    }
  }
  MORSE_DDR |= _BV(MORSE_BIT);
  MORSE_PORT &= ~_BV(MORSE_BIT);

  for ( ; ; ) {
    int16_t mag[3];
    int8_t done;

    if( (e = cmpas_read_raw(mag)) )return(e);

    tx_puts("## ");
    tx_puts(fmt_i16d(mag[0])); tx_putc(' ');
    tx_puts(fmt_i16d(mag[1])); tx_putc(' ');
    tx_puts(fmt_i16d(mag[2])); tx_putc(' ');
    tx_puts(fmt_i16d(saved[0])); tx_putc(' ');
    tx_puts(fmt_i16d(saved[1])); tx_putc(' ');
    tx_puts(fmt_i16d(saved[2])); tx_puts("\r\n");

    for(ax = 0; ax < 3; ax++)
      if( mag[ax] > 2047 || mag[ax] < -2048 ) {
        tx_msg("## out of range: ", mag[ax]);
        goto discard;
      }

#define MIN_SAVES_REQD (ARYSZ*24)
    done = 0;
    for(ax = 0; ax < 3; ax++) {
      int8_t s;
      if (saved[ax] >= MIN_SAVES_REQD) done++;
      s = savepoints(mag[ax], axes + ax);
      if (!s) continue;
      MORSE_PORT |= _BV(MORSE_BIT);
      saved[ax]++;
      tx_msg("Saved on axis ", ax);
      MORSE_PORT &= ~_BV(MORSE_BIT);
    }
    if (done == 3) break;

discard: ;
  }

  tx_puts("calib.cmpas_offsets:");
  for(ax = 0; ax < 3; ax++){
    int16_t smallest, biggest, pos;
    /* Use LEAST extreme value as offset, assume others are outliers */
    pos = max(axes[ax].min);
    smallest = axes[ax].min[pos];
    pos = min(axes[ax].max);
    biggest = axes[ax].max[pos];
    calib.cmpas_offsets[ax] = 0 - (smallest + biggest)/2;
    calib.cmpas_scale[ax] = 24576/((biggest - smallest)/2);
    tx_msg("Axis ", ax);
    tx_msg("Offset = ", calib.cmpas_offsets[ax]);
    tx_msg("Scale = ", calib.cmpas_scale[ax]);
    tx_msg("Min = ", smallest);
    tx_msg("Max = ", biggest);
  }

  ee_store(calib.cmpas_offsets, ee.tude.cmpas_offsets,
    sizeof(ee.tude.cmpas_offsets));
  ee_store(calib.cmpas_scale, ee.tude.cmpas_scale,
    sizeof(ee.tude.cmpas_scale));

  MORSE_PORT |= _BV(MORSE_BIT);
  return(0);
}
