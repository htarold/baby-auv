/*
  (C) 2019 Harold Tay LGPLv3
  LSM303:
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
#include "ee.h"                       /* for calibration values */

#define LSM303DLHC

#undef  DBG_LSM303
#ifdef DBG_LSM303
#define PRINT(a) a
#else
#define PRINT(a) /* nothing */
#endif

static struct tude calib;  /* contains calibration offsets */

static int8_t load_offsets(int16_t * ary, int16_t * addr)
{
  if (ee_load(ary, addr, sizeof(calib.cmpas_offsets))) {
    uint8_t i;
    for(i = 0; i < 3; i++) ary[i] = 0;
    /* is uncalibrated, calibration is REQUIRED */
    return(1);
  }
  return(0);
}

/*
  Accelerometer addresses are different for DLH and DLHC.
 */
#define ACCEL_ADDR_DLH (0x18<<1)
#define ACCEL_ADDR_DLHC (0x19<<1)
#define CMPAS_ADDR (0x1E<<1)
static uint8_t accel_addr;

static int8_t accelinit(int8_t addr)
{
  int8_t e;

  i2c_init();
  if( (e = i2c_setreg(addr, 0x20, 0x37)) )return(e);
  /* CANNOT set small/native endian, DLHC bug. */
  e = i2c_setreg(addr, 0x23, addr == ACCEL_ADDR_DLHC?0x1:0);
  return(e);
}
int8_t accel_init(void)
{
  int8_t e;
  e = accelinit(accel_addr = ACCEL_ADDR_DLH);
  if (e) e = accelinit(accel_addr = ACCEL_ADDR_DLHC);
  if (e) return(e);
  return(load_offsets(calib.accel_offsets, ee.tude.accel_offsets));
}
static void reorient(int16_t ary[3])
{
  int16_t tmp[3];
  tmp[0] = AUVXFRONT(ary);
  tmp[1] = AUVYRIGHT(ary);
  tmp[2] = AUVZDOWN(ary);

  ary[0] = tmp[0];
  ary[1] = tmp[1];
  ary[2] = tmp[2];
}
static int8_t read6(uint8_t addr, uint8_t reg, uint16_t ary[3])
{
  int i;
  uint8_t buf[6];
  i = i2c_read(addr, reg, buf, 6);
  if( i )return(i);
  for(i = 0; i < 3; i++) {
    ary[i] = (buf[(i*2)]<<8) | (buf[(i*2)+1] & 0xff);
  }
  return(0);
}

int8_t accel_read_raw(int16_t ary[3])
{
  int8_t e;
  uint16_t tmp[3];
#define AUTO_INCREMENT (1<<7)
  e = read6(accel_addr, 0x28|AUTO_INCREMENT, tmp);
  if( e )return(e);
  /* convert endian (accel only) */
  ary[0] = ((tmp[0] << 8) & 0xff00) | ((tmp[0] >> 8) & 0xff);
  ary[1] = ((tmp[1] << 8) & 0xff00) | ((tmp[1] >> 8) & 0xff);
  ary[2] = ((tmp[2] << 8) & 0xff00) | ((tmp[2] >> 8) & 0xff);
  ary[0] /= 16;
  ary[1] /= 16;
  ary[2] /= 16;
  return(0);
}

int8_t check_magnitude(uint16_t nom2, int16_t ary[3])
{
  uint16_t magnitude2;
  uint8_t i;
  magnitude2 = (ary[0]/8)*(ary[0]/8)
             + (ary[1]/8)*(ary[1]/8)
             + (ary[2]/8)*(ary[2]/8);
  if( magnitude2 > nom2 )magnitude2 -= nom2;
  else magnitude2 = nom2 - magnitude2;
  nom2 -= nom2/4;
  for(i = 0; i < 8; i++){
    if( magnitude2 >= nom2 )break;
    magnitude2 <<= 1;
  }
  return(i);
}
int8_t accel_read(int16_t ary[3])
{
  int8_t e, i;
  e = accel_read_raw(ary);
  if( e )return(e);
  for(i = 0; i < 3; i++){
    ary[i] += calib.accel_offsets[i];
  }
  reorient(ary);
#if 0
  /*
    Warn if acceleration != 1g.
    Scaled so that 1g = 1024/8 = 128
    Compare the square: should be close to 128*128 = 16384
   */
  return(check_magnitude(16384, ary));
#else
  return(0);
#endif
}

static uint8_t cmpas_xzy;

int8_t cmpas_init(void)
{
  int8_t e;
  uint8_t b;

  /*
    Compass gain max = 1<<5, min = 7<<5
   */
  i2c_init();
  PRINT(tx_puts("## setreg 1...\r\n"));
  if( (e = i2c_setreg(CMPAS_ADDR, 0x00, 0x14)) )return(e); /* 30Hz */
  PRINT(tx_puts("## setreg 2...\r\n"));
  if( (e = i2c_setreg(CMPAS_ADDR, 0x01, 7<<5)) )return(e); /* gain */
  PRINT(tx_puts("## setreg 3...\r\n"));
  if( (e = i2c_setreg(CMPAS_ADDR, 0x02, 0x00)) )return(e); /* continuous */

  b = 0;
  PRINT(tx_puts("## i2c_read...\r\n"));
  e = i2c_read(CMPAS_ADDR, 0x0f, &b, 1);  /* xyz/xzy? */
  if (e) return(e);
  cmpas_xzy = ('<' == b);
  PRINT(tx_msg("who_am_i_m: ", b));

  return
    load_offsets(calib.cmpas_offsets, ee.tude.cmpas_offsets) |
    load_offsets(calib.cmpas_scale, ee.tude.cmpas_scale);
}

int8_t cmpas_read_raw(int16_t ary[3])
{
  int8_t e;
  e = read6(CMPAS_ADDR, 0x03, (uint16_t*)ary);
  if (e) return(e);
  if (cmpas_xzy) {
    int16_t y;
    /* positions of z and y are exchanged */
    y = ary[2]; ary[2] = ary[1]; ary[1] = y;
  }
  return(0);
}
int8_t cmpas_read(int16_t ary[3])
{
  int i;
  if( (i = cmpas_read_raw(ary)) )return(i);
  for(i = 0; i < 3; i++) {
    ary[i] += calib.cmpas_offsets[i];
    ary[i] *= calib.cmpas_scale[i];
    ary[i] /= 128;  /* approx. value of scale */
  }
  reorient(ary);
  /*
    Normal magnetic field = 0.25G (269lsb at gain = 1.9) to
    0.65G (744lsb at gain = 1.9).
   */
#if 0
  return(check_magnitude(4100, ary));
#else
  return(0);
#endif
}
