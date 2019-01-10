/* (C) 2019 Harold Tay LGPLv3 */
#include "imaths.h"
#include "tude.h"

#undef DEBUG
#ifdef DEBUG
#include "tx.h"
#define DBG(x) x
#else
#define DBG(x)
#endif

/* about 1.4ms using isqrt, 418us using icos(iasin()) */
static void calculate_attitude(struct angles * tp, int16_t a[3], int16_t m[3])
{
  int16_t mr[2];                      /* rotated magnetic vector */
  int16_t sp, cp;                     /* sin, cos of pitch angle */
  int16_t sr, cr;                     /* sin, cos of roll angle */
  int32_t tmp1, tmp2;

  /*
    Denominator is implicitly 1024
   */

  sp = a[0];
  cp = 0;

  if (sp > 1024)
    sp = 1024;
  else if (sp < -1024)
    sp = -1024;
  else {
    cp = icos1024(iasin(sp));
  }

  if( cp ){
    sr = ((int32_t)a[1]*1024L)/cp;
    if( a[1] < 1024 && a[1] > -1024 ){
      cr = icos1024(iasin(sr));
    }else
      cr = 0;
  }else{
    sr = 0;
    cr = 1024;
  }

  if (m) {

    /*
      Rotate magnetic vector
      Normal magnetic field magnitude will be around 400LSB.
     */
#define M(i) ((int32_t)(m[i]))
    tmp1 = (M(0)*(int32_t)cp);
    DBG(tx_puts("mr_0_tmp1 = "));
    DBG(tx_putdec32(tmp1));
    DBG(tx_puts("\r\n"));
    tmp2 = (M(2)*(int32_t)sp);
    DBG(tx_puts("mr_0_tmp2 = "));
    DBG(tx_putdec32(tmp2));
    DBG(tx_puts("\r\n"));
    mr[0] = (int16_t)((tmp1 + tmp2)/1024L);

    tmp1 = ((int32_t)sr*(int32_t)sp)/1024L;
    tmp1 *= M(0);
    DBG(tx_puts("mr_1_tmp1 = "));
    DBG(tx_putdec32(tmp1));
    DBG(tx_puts("\r\n"));
    tmp2 = ((int32_t)sr*(int32_t)cp)/1024L;
    tmp2 *= M(2);
    DBG(tx_puts("mr_1_tmp2 = "));
    DBG(tx_putdec32(tmp2));
    DBG(tx_puts("\r\n"));
    tmp1 -= tmp2;
    tmp1 += M(1)*cr;
    mr[1] = (int16_t)(tmp1 / 1024L);

    /* XXX Sense is reversed, reasons unknown */
    tp->heading = 0 - iatan2(mr[1], mr[0]);
    DBG(tx_msg("## cp =", cp));
    DBG(tx_msg("## mr[1]=", mr[1]));
    DBG(tx_msg("## mr[0]=", mr[0]));
    DBG(tx_msg("## hdg = ", tp->heading));
  } else
    tp->heading = 0;
  /* XXX Sense is reversed, reasons unknown */
  tp->roll = 0 - iatan2(sr, cr);
  tp->sin_pitch = sp;
}

static struct angles angles;

static inline int8_t refresh_tude(void)
{
  int16_t a[3], m[3];
  int8_t er;

  er = accel_read(a);
  if (er) {
    DBG(tx_msg("Err:refresh_tude:accel_read=", er));
    return(er);
  }
  er = cmpas_read(m);
  if (er) {
    DBG(tx_msg("Err:refresh_tude:cmpas_read=", er));
    return(er);
  }
  calculate_attitude(&angles, a, m);

  return(0);
}

int8_t tude_read(struct angles * tp)
{
  int8_t er;

  er = refresh_tude();
  if (er) return(er);

  *tp = angles;
  return(0);
}

