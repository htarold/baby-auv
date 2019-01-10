/*
  (C) 2019 Harold Tay LGPLv3
  Combining tude, bat, and depth tasks.
  This is the bg task to be normally used.
 */

#include <avr/interrupt.h>            /* for cli() */
#include "tude.h"
#include "bat.h"
#include "ctd.h"
#include "syslog.h"
#include "time.h"
#include "yield.h"
#include "thrust.h"
#include "bg.h"
#include "odo.h"
#include "mma.h"
#include "handydefs.h"

#define DEBUG
#ifdef DEBUG
#include "tx.h"
#include <util/delay.h>
#define SYSLOG_ATTR(a,b) tx_msg(a, b)
#define SYSLOG_LATTR(a,b) tx_msg(a, b)
#define DBG(x) x
#else
#define SYSLOG_ATTR(a,b) syslog_attr(a, b)
#define SYSLOG_LATTR(a,b) syslog_lattr(a, b)
#define DBG(x) /* nothing */
#endif

static int8_t bg_init(void)
{
  int8_t er;
  thrust_init();
  er = accel_init();
  if (er) {
    syslog_attr("Err:Accel_Init", er);
    return(er);
  }
  er = cmpas_init();
  if (er) {
    syslog_attr("Err:Cmpas_Init", er);
    return(er);
  }
  er = ctd_init();
  if (er) {
    syslog_attr("Err:ctD_Init", er);
    return(er);
  }
  odo_init();
  bg_pitch_set(BG_PITCH_INVALID, 0);
  thrust_start_rpm_count();
  return(0);
}

static uint8_t prescaler_shifts = 0;

/*
  Slow everything down.
 */
void bg_task_prescaler(uint8_t shifts)
{
  if (shifts < 4)
    prescaler_shifts = shifts;
}

struct angles pub_angles;
int16_t pub_mv, pub_ma, pub_cm;
uint16_t pub_degc;
uint32_t pub_us;
uint16_t pub_rpm;
uint16_t pub_revs_x2;
uint8_t pub_updated;

/*
  Central point to get AUV attitude.
  This can be done for attitude since it will return
  synchronously.  Cannot be done for CTD, which will yield.
 */
int8_t bg_attitude(void)
{
  int er;
  struct angles angles;
  er = tude_get(&angles, TUDE_RATE_FAST);
  if (er) {
    syslog_attr("Err:Tude_Get", er);
    return(er);
  }
  pub_angles = angles;
  pub_updated ^= (1<<PUB_ANGLES);
  syslog_attr("hdg", pub_angles.heading);
  syslog_attr("sth", pub_angles.sin_pitch);
  syslog_attr("phi", pub_angles.roll);
  return(0);
}

static uint8_t mma_recently;          /* increments if mma is idle */
static void log_mma_set(int8_t percent)
{
  mma_set(percent);
  syslog_attr("mma", percent);
  mma_recently = 0;                   /* start the countup */
}
/*
  Centralise access to mma_set to consolidate syslogging.
 */
void bg_mma(int8_t percent)
{
  log_mma_set(percent);
  bg_pitch_set(BG_PITCH_INVALID, 0);
}

/* The sine of the angle is used, not the angle */
static int16_t bg_pitch_sp;
static int16_t bg_pitch_tol;
void bg_pitch_set(int16_t pitch_sp, int16_t pitch_tol)
{
  if (BG_PITCH_INVALID == pitch_sp ||
      (pitch_sp <= 1024 && pitch_sp >= -1024)) {
    bg_pitch_sp = pitch_sp;
    bg_pitch_tol = pitch_tol;
    if (bg_pitch_tol < 0)
      bg_pitch_tol = 0 - bg_pitch_tol;
  }
}

/* called every 2 seconds, after bg_attitude() */
static void pitch_slow_controller(void)
{
  int16_t error, abs_er;

  if (mma_recently > -1)
    if (mma_recently < 100)           /* don't overflow */
      mma_recently++;                 /* count up to mma_stop() */

  if (BG_PITCH_INVALID == bg_pitch_sp)
    return;

  if (mma_recently < 2)               /* let settle after an action */
    return;

  error = pub_angles.sin_pitch - bg_pitch_sp;
  abs_er = (error<0?0-error:error);
  if (abs_er < bg_pitch_tol) return;

  error /= 16;
  if (!error) return;
  CONSTRAIN(error, -5, 5);

  error += mma_get();
  CONSTRAIN(error, -100, 100);
  log_mma_set(error);
  syslog_attr("bg_pitch_sp", bg_pitch_sp);
  syslog_attr("bg_pitch_ctrl", error);
}

/* Keep ctd_setup() and ctd_read() together, maintain sync */
static int8_t bg_ctd(uint8_t which, int16_t * raw)
{
  const static char names[3][12] PROGMEM = {
    [CTD_DEPTH] = "ErrcTdDepth",
    [CTD_COND]  = "ErrcTdCond",
    [CTD_THERM] = "ErrcTdTherm",
  };
  int8_t er;
  er = ctd_setup(which);
  if (er) goto bomb;
  yield();
  yield();
  er = ctd_read(raw);
  if (er) goto bomb;
  return(0);
bomb:
  syslog_attrpgm(names[which], er);
  return(er);
}

static void bg_rpm(void)
{
#define NR_PERIODS 3                  /* 1 period is 2 seconds.  */
  static uint16_t revs_x2[NR_PERIODS+1];
  int16_t nr_transitions;
  int8_t i;

  for (i = NR_PERIODS; i > 0; i--)
    revs_x2[i] = revs_x2[i-1];
  revs_x2[0] = thrust_get_revs_x2();

  nr_transitions = revs_x2[0] - revs_x2[NR_PERIODS];
  if (nr_transitions < 0) return;     /* Can't be arsed with overflow */
  /*
    RPM = ((nr_transitions/2)/(NR_PERIODS*2seconds)) * 60
   */
  pub_rpm = nr_transitions * (60/(2*NR_PERIODS*2));
  syslog_attr("rpm", pub_rpm);
  pub_updated ^= (1<<PUB_RPM);
  syslog_attr("revs_x2", revs_x2[0]);
  pub_updated ^= (1<<PUB_REVS_X2);
}

void bg_task(void)
{
  int8_t er;

  er = bg_init();
  syslog_attr("bg_init", er);
  if (er)
    panic("Err:BGtask", er);

  for ( ; ; ) {
    uint8_t short_clock;
    /* Let it through only at the top of the second */
    do { yield(); } while (time_100s);

    short_clock = (time_uptime >> prescaler_shifts) & 0xff;

#define EVERY(sec,offset) \
if ((sec-1) == ((short_clock+offset) & (sec-1)))

    EVERY(2,1) {
      bg_rpm();
      (void)bg_attitude();
      pitch_slow_controller();
    }

    EVERY(2,0) {
      int16_t raw;
      pub_mv = bat_voltage();
      pub_ma = bat_current();
      syslog_attr("mv", pub_mv);      /* Battery EMF, millivolts */
      syslog_attr("ma", pub_ma);      /* Battery draw, milliamps */
      pub_updated ^= (1<<PUB_BAT);

      ctd_start();
      yield();                        /* wait for CTD to power up */
      if (0 == bg_ctd(CTD_DEPTH, &raw)) {
        syslog_attr("cm_raw", raw);
        ctd_calc_depth(raw, &pub_cm);
        syslog_attr("cm", pub_cm);      /* Depth in centimetres */
        pub_updated ^= (1<<PUB_CM);
        yield();
      }

      EVERY(4,0) {
        if (0 == bg_ctd(CTD_COND, &raw)) {
          syslog_attr("us_raw", raw);
          ctd_calc_conductance(raw, &pub_us);
          syslog_lattr("us", pub_us);
          pub_updated ^= (1<<PUB_US);
          yield();
        }
        if (0 == bg_ctd(CTD_THERM, &raw)) {
          syslog_attr("degc_raw", raw);
          ctd_calc_resistance(raw, &pub_degc);
          syslog_lattr("degc", pub_degc);
          pub_updated ^= (1<<PUB_DEGC);
          yield();
        }
      }
      ctd_stop();
    }

    EVERY(8,1) {
      odo_periodically();
      syslog_putlit(" E ");           /* buzz a bit */
    }

    EVERY(16,1) {
      if (mma_recently > 8) {
        /* Idle for long enough, shut off */
        mma_stop();
        syslog_lattr("mma_stop", mma_recently);
        mma_recently = -1;
      }
    }
  }
}
