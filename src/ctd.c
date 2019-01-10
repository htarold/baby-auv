#include <avr/io.h>
#include "ctd.h"
#include "ads1015.h"
#include "i2c.h"
#include <util/delay.h>

#define DEBUG
#ifdef DEBUG
#include "tx.h"
#define DBG(x) x
#else
#define DBG(x) /* nothing */
#endif

/*
  (C) 2019 Harold Tay LGPLv3
  Always uses PGA setting of 2048mV.  For MPXxxxxAP pressure
  sensor, output is 0-5V but circuitry scales this to 0-2.x
  volts.  Thermistor and conductivity cell are excited to 2.45V,
  but response will be approx. half this value, so will be
  within range.

  Sampling at 250SPS (conservative choice).

  New channel assignments: depth = AIN0, thermistor = AIN1,
  conductivity = AIN2, v_half = AIN3.
 */

struct ctd ctd;                       /* calibrations */

#define AT24_ADDR 0x50
#define CTD_DATA_START 0

/* Read or write to the eeprom */
static int8_t eeprom_io(uint8_t addr)
{
  int8_t er, i, j;
  uint8_t * p;

  p = (uint8_t *)&ctd;

  for (i = 0; i < sizeof(ctd); i++) {
    for (j = 0; ; j++) {
      if (addr & 1)
        er = i2c_read(addr&(~1), CTD_DATA_START + i, p + i, 1);
      else
        er = i2c_setreg(addr, CTD_DATA_START + i, p[i]);
      if (!er) break;
      if (j >= 10) {
        DBG(tx_msg("## eeprom_io failed ", er));
        return(er);
      }
      _delay_us(100);
    }
  }
  return(0);
}

int8_t ctd_store_eeprom(void) { return(eeprom_io(AT24_ADDR<<1)); }

int8_t ctd_load_eeprom(void) { return(eeprom_io((AT24_ADDR<<1)|1)); }

int8_t ctd_init(void)
{
  int8_t er;
  ctd_start();
  _delay_ms(50);
  er = ctd_load_eeprom();
  ctd_stop();
  return(er);
}
void ctd_start(void)
{
  DDRD |= _BV(PD5);                   /* PD5 is OC0B */
  TCCR0A = _BV(WGM01)                 /* CTC mode 2 */
         | _BV(COM0B0);               /* Toggle on match */
  TCCR0B = _BV(CS01)|_BV(CS00);       /* /64 */
#define HZ (F_CPU/64)
  /*
    Excitation frequency will alter the readings: Too high and
    duty cycle on board becomes asymmetric.  1kHz is ok.
   */
#define EXCITATION_HZ 1000L           /* Min value 490Hz */
  OCR0A = ((HZ/2)/EXCITATION_HZ)-1;   /* TOP value */
  OCR0B = 0;
}

void ctd_stop(void)
{
  /* TCCR0B = 0; XXX leave it always on */
}

#define CFG_DEF \
ADS1015_CFG_OS | ADS1015_MODE_1SHOT|ADS1115_SPS_250\
|ADS1015_COMP_REGULAR|ADS1015_COMP_ACTIVE_LOW|ADS1015_COMP_NOLATCH\
|ADS1015_COMP_NEVER|ADS1015_PGA_1024MV

/* Differential mode reduces tempco:  */
#define CFG_DEPTH (CFG_DEF | ADS1015_MUX_AIN0AIN3)
#define CFG_THERM (CFG_DEF | ADS1015_MUX_AIN1AIN3)
#define CFG_COND  (CFG_DEF | ADS1015_MUX_AIN2AIN3)

int8_t ctd_setup(uint8_t which)
{
  static uint16_t cfgs[3] = { CFG_DEPTH, CFG_COND, CFG_THERM };

  if (which > 2) return(-1);
  return ads1015_setup(CTD_ADDR, cfgs[which]);
}
int8_t ctd_read(int16_t * p)
{
  return ads1015_get(CTD_ADDR, p);
}
int8_t ctd_busy(void)
{
  uint16_t cfg;
  int8_t er;
  er = ads1015_check(CTD_ADDR, &cfg);
  if (er) return(er);
  if (cfg & ADS1015_CFG_OS) return(0); /* Is not busy */
  return(1); /* Is busy */
}

int8_t ctd_calc_depth(int16_t raw, int16_t * dp)
{
  int32_t cm;
  cm = (raw - (int32_t)ctd.depth_offset) * (ctd.depth_gain1024);
  cm /= 1024;
  *dp = (int16_t)cm;
  return(0);
}

int8_t ctd_get_depth(int16_t * dp)
{
  int8_t er;
  int16_t raw;

  er = ctd_read(&raw);
  if (!er) er = ctd_calc_depth(raw, dp);
  return(er);
}

int8_t ctd_get_resistance(uint16_t * rp)
{
  int16_t raw;
  int8_t er;
  uint32_t num, den;

  er = ctd_read(&raw);
  if (er) return(er);

  /*
    AIN1/AIN3  Assume AIN3 is at 1/2 potential = 0.9V or
    (0.9/2.048)*65536 = 28800 counts.
    AIN1 is at V = 1.8*R/(R+10k)
    or ((1.8*R/(R+10k))/2.048)*65536 counts
    = 57600 * R/(R+10k), R = counts * 10k/(57600 - counts)
    where counts is single-ended.
    i.e. raw = counts - 28800, counts = raw + 28800
    R = (raw + 28800)*10k/(57600 - raw - 28800)
    R = (raw + 28800)*10k/(28800 - raw)
   */
  num = (raw + 28800)*10000;
  den = 28800 - raw;
  *rp = num/den;
  return(0);
}

/*
  Calculate thermistor resistance without calibration.
  uint16_t should be good down to -11 degC.
 */
int8_t ctd_calc_resistance(int16_t raw, uint16_t * rp)
{
  int32_t r, den, c_raw;

  den = (int32_t)ctd.c_vcc - (int32_t)raw;
  if (!den) return(-1);
  c_raw = raw - ctd.c_gnd;
  r = ((int32_t)ctd.therm_r * c_raw)/den;
  *rp = (uint16_t)r;
  return(0);
}

/*
  Basic formula:
  R+R2 = R1*c_raw/(c_vcc - c_raw)
  where c_* are all relative to ctd.cond_c_gnd (i.e. c_vcc =
  ctd.cond_c_vcc - ctd.cond_c_gnd, etc.)
  SCALE macro is used to extract some extra resolution from
  the arithmetic.  This mostly helps in the seawater case, not
  so much if conductivity is low.  SCALE of 32 is safe.
 */
int8_t ctd_calc_conductance(int16_t raw, uint32_t * gp)
{
  int32_t r, den, c_raw;
  c_raw = (int32_t)raw - ctd.c_gnd;
  /*
    Scale by 32 if low conductance, by 64 if high conductance:
   */
#define SCALE_LOW 32L
#define SCALE_HIGH 64L
  if (raw > 1000) {
    r = (ctd.cond_r_1 * c_raw)*SCALE_LOW;
    den = (((int32_t)ctd.c_vcc - (int32_t)ctd.c_gnd) - c_raw);
    r /= den;
    r -= ctd.cond_r_2*SCALE_LOW;
    *gp = SCALE_LOW*1000000L/r;
  } else {
    r = (ctd.cond_r_1 * c_raw)*SCALE_HIGH;
    den = (((int32_t)ctd.c_vcc - (int32_t)ctd.c_gnd) - c_raw);
    r /= den;
    r -= ctd.cond_r_2*SCALE_HIGH;
    *gp = SCALE_HIGH*1000000L/r;
  }
  return(0);
}

int8_t ctd_get_conductance(uint32_t * gp)
{
  int8_t er;
  int16_t raw;

  er = ctd_read(&raw);
  if (er) return(er);

  return(ctd_calc_conductance(raw, gp));
}

