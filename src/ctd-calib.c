/*
  (C) 2019 Harold Tay GPLv3
  Currently only doing depth.
 */
#include "ctd.h"
#include "tx.h"
#include "rx.h"
#include "i2c.h"
#include "time.h"
#include <util/delay.h>
#include <avr/interrupt.h>
#include <util/twi.h>
#if 1
#define DBG(x) x
#else
#define DBG(x) /* nothing */
#endif

/*
  EEPROM operations: Write is only for calibration.  Assumes WP
  pin is always at gnd (not write protected).
 */

extern struct ctd ctd;

static void delay(void) { _delay_ms(500); }

static char getchar(void)
{
  while (!rx_havechar()) ;
  return(rx_getchar());
}
static uint8_t yes(void)
{
  char ch;
  ch = getchar();
  while (rx_havechar()) rx_getchar();
  if (ch == 'y' || ch == 'Y') return(1);
  return(0);
}

static int8_t get_reading(uint8_t which, int16_t * rp)
{
  int8_t er;
  er = ctd_setup(which);
  if (er) {
    tx_strlit("\r\nctd_setup error = ");
    tx_putdec(er);
    return(-1);
  }
#if 0
  for ( ; ; ) {
    er = ctd_busy();
    if (1 == er) continue;
    if (0 == er) break;
    tx_strlit("\r\nctd_busy error = ");
    tx_putdec(er);
    return(-1);
  }
#else
  _delay_ms(9);
#endif
  er = ctd_read(rp);
  if (er) {
    tx_strlit("\r\nctd_read error = ");
    tx_putdec(er);
    return(-1);
  }
  return(0);
}

static int16_t select_reading(uint8_t which)
{
  int16_t d;

  tx_strlit("<enter> when values stabilise:\r\n");

  for ( ; ; ) {
    if (get_reading(which, &d)) for ( ; ; ) ;
    tx_putdec(d);
    tx_strlit("      \r");
    if (rx_havechar()) break;
    _delay_ms(500);
  }

  do { rx_getchar(); } while (rx_havechar());
  tx_strlit("\r\n");
  return(d);
}

#define ASK(lit, old) \
({ const static char s[] PROGMEM = lit; ask(s, old); })

static int16_t ask(const char * msg, int16_t oldval)
{
  int16_t val;
  int8_t i, neg;

  tx_putpgms(msg);
  tx_putdec(oldval);
  tx_strlit("\r\nEnter new value: ");
  neg = 0;
  val = 0;
  for (i = 0; ; i++) {
    char ch;
    ch = getchar();
    tx_putc(ch);
    if ('-' == ch) neg = 1;
    if (ch < '0' || ch > '9') break;
    val *= 10;
    val += (ch - '0');
  }
  if (i <= 0) return(oldval);         /* no change */
  if (neg) return(0 - val);
  return(val);
}

void ctd_depth_calibrate(void)
{
  int8_t er;
  int16_t d;

  tx_strlit("Depth previous calibration: depth_gain1024 = ");
  tx_putdec(ctd.depth_gain1024);
  tx_strlit(", depth_offset = ");
  tx_putdec(ctd.depth_offset);
  tx_strlit("\r\nAbort to retain previous calibration.\r\n");

  tx_strlit("Raw reading at depth 0m: ");
  ctd.depth_offset = select_reading(CTD_DEPTH);


  tx_strlit("Apply 10m of pressure:\r\n");
  d = select_reading(CTD_DEPTH);
  /* depth = (raw - depth_offset)*(depth_gain1024/1024) */
  ctd.depth_gain1024 = (
                       (long)10       /* metres */
                       * 100          /* cm */
                       )
                       * 1024         /* scale */
                       / (d - ctd.depth_offset);

  er = ctd_store_eeprom();
  if (er) {
    tx_msg("Fatal error writing to eeprom: ", er);
    for ( ; ; );
  }
  tx_strlit("\r\ndepth_offset = ");
  tx_putdec(ctd.depth_offset);
  tx_strlit("\r\ndepth_gain1024 = ");
  tx_putdec(ctd.depth_gain1024);
  tx_strlit("\r\nDepth calibration complete.\r\n");

  ctd_stop();
  ctd_start(); /* Force reload of calibration values */

  tx_strlit("Depth in cm:\r\n");
  for ( ; ; ) {
    int16_t d;
    ctd_setup_depth();
    delay();
    ctd_get_depth(&d);
    tx_putdec(d);
    tx_strlit("    \r");
    if (rx_havechar()) break;
  }
  do { rx_getchar(); } while (rx_havechar());
}

void ctd_therm_calibrate(void)
{
  tx_strlit("\r\nOpen thermistor counts, old reading = ");
  tx_putdec(ctd.c_vcc);
  tx_strlit(" Get new value? ");
  if (yes()) {
    tx_strlit(" remove the thermistor, ");
    ctd.c_vcc = select_reading(CTD_THERM);
  }
  tx_strlit("\r\nShorted thermistor counts, old reading = ");
  tx_putdec(ctd.c_gnd);
  tx_strlit(" Get new value? ");
  if (yes()) {
    tx_strlit(" short the thermistor, ");
    ctd.c_gnd = select_reading(CTD_THERM);
  }

  ctd.therm_r = ASK("Divider resistor value (ohms) = ", ctd.therm_r);

  if (ctd_store_eeprom()) {
    tx_strlit("\r\nFatal ctd_store_eeprom error\r\n");
    for ( ; ;);
  }

  for ( ; ; ) {
    int16_t raw;
    int8_t er;
    uint16_t resistance;
    er = get_reading(CTD_THERM, &raw);
    if (er) {
      tx_strlit("\rError reading thermistor");
      goto delay;
    }
    er = ctd_calc_resistance(raw, &resistance);
    if (er) {
      tx_strlit("\rError calculating thermistor resistance");
      goto delay;
    }
    tx_strlit("\rThermistor resistance (raw,ohms) = ");
    tx_putdec(raw);
    tx_putc(',');
    tx_putdec32((int32_t)resistance);
    tx_strlit("   ");
delay:
    delay();
  }
}

/*
  XXX This is partial calibration.  Full calib to follow.
 */

void ctd_cond_calibrate(void)
{
  int8_t er;
  int16_t t1, t2;

#define HZ F_CPU/8
  ctd.cond_exc_hz = (uint16_t)((HZ/2)/(OCR0A+1));
  tx_strlit("\r\nCalibrating at frequency (Hz) = ");
  tx_putdec(ctd.cond_exc_hz);
  tx_strlit("\r\n");

  ctd.cond_r_1 =
    ASK("Upper resistor value (R1, ohms) = ", ctd.cond_r_1);
  ctd.cond_r_2 =
    ASK("Lower resistor value (R2, ohms) = ", ctd.cond_r_2);

#if 0
  tx_strlit("\r\nOpen cell counts, old reading = ");
  tx_putdec(ctd.cond_c_vcc);
  tx_strlit(" Get new value? ");
  if (yes()) {
    tx_strlit(" remove the cell, ");
    ctd.cond_c_vcc = select_reading(CTD_COND);
  }

  tx_strlit("\r\nShorted cell counts, old reading = ");
  tx_putdec(ctd.cond_c_0);
  tx_strlit(" Get new value? ");
  if (yes()) {
    tx_strlit(", short the cell, ");
    ctd.cond_c_0 = select_reading(CTD_COND);
  }

  /*
    Calculate c_gnd (which will be negative):
    c_gnd = c_0 - (R2*(c_vcc - c_0)/R1)
   */
  c = (float)ctd.cond_c_vcc - (float)ctd.cond_c_0;
  c *= (float)ctd.cond_r_2;
  c /= (float)ctd.cond_r_1;
  c = (float)ctd.cond_c_0 - c;
  ctd.cond_c_gnd = (int16_t)c;
  tx_strlit("\r\ncalculated ground potential = ");
  tx_putdec(ctd.cond_c_gnd);
#endif

  er = ctd_store_eeprom();
  if (er)
    for ( ; ; ) {
      tx_strlit("\r\nFatal ctd_store_eeprom error = ");
      tx_putdec(er);
      _delay_ms(500);
    }

  tx_strlit("\r\n(Partial) calibration done.\r\n");

  for ( ; ; ) {
    uint32_t cond;
    int16_t raw;

    if (get_reading(CTD_COND, &raw)) goto delay;

    t1 = TCNT1;
    er = ctd_calc_conductance(raw, &cond);
    t2 = TCNT1;
    if (er) {
      tx_strlit("\r\nctd_get_conductance error ");
      tx_putdec(er);
      goto delay;
    }
    if (t1 > t2)
      t2 += TIMER_TOP + 1;
    t2 -= t1;
    tx_strlit("\rconductance (raw,microsiemens) = ");
    tx_putdec(raw);
    tx_putc(',');
    tx_putdec32(cond);
    tx_strlit("    Elapsed = ");
    tx_putdec(t2);
    tx_strlit(" us   ");

delay:
    _delay_ms(500);
  }
}

int
main(void)
{
  int8_t i, er;

  time_init(); /* only for profiling */
  tx_init();
  rx_init();
  rx_enable();
  sei();

  for (i = 3; i > 0; i--) {
    delay();
    delay();
    tx_puts("\r\nDelay ");
    tx_putdec(i);
  }
  tx_puts("\r\n");

  i2c_init();
  er = ctd_init();
  if (er) for ( ; ; ) {
    tx_msg("ctd_init error: ", er);
    _delay_ms(100); /* XXX not in milliseconds! */
  }
  ctd_start();
  _delay_ms(500);

  er = 0;
#if 0
  /*
    Check writing to eeprom.
   */
  ctd.depth_offset = 201;
  er = ctd_store_eeprom();  
  tx_msg("store_eeprom = ", er);
  ctd.depth_offset = 9;

  /*
    Check reading from eeprom.
   */
  er = ctd_load_eeprom();
  tx_msg("load_eeprom = ", er);
  tx_msg("##ctd.depth_offset = ", ctd.depth_offset);
#endif /* 0 */

  tx_strlit("\r\nNote: Must calibrate thermistor before conductivity.");
  tx_strlit("\r\nCalibrate thermistor?  ");
  if (yes()) ctd_therm_calibrate();
  tx_strlit("\r\nCalibrate conductivity?  ");
  if (yes()) ctd_cond_calibrate();
  tx_strlit("\r\nCalibrate depth?  ");
  if (yes()) ctd_depth_calibrate();
  tx_puts("\r\nDone.\r\n");
  for ( ; ; );
}
