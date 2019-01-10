/*
  (C) 2019 Harold Tay LGPLv3
  AUV battery voltage and current using on-board ADC.
  Vbat = ADC7, 47k/10k divider.

  Board has AVCC wired to 3.3V, and AREF decoupled with
  capacitor (REFS1:0 = 01).

  Transconductance Gm of ZXCT1009 is 10000uA/V = 0.01A/V
  Ibat = 3.3 * lsb / (1024 * Ro * Gm * Rs)
 */

#include <avr/io.h>
#include <util/delay.h>

#define MUX_VBAT 0x7 /* ADC7 */
#define MUX_IBAT 0x6 /* ADC6 */

#define ADC_RESOLUTION 1024
#define SENSE_RESISTANCE 0.22
#define OUTPUT_RESISTANCE 1500.0
#define ZXCT_GM 0.01 /* A/V */
#define VCC 3.3 /* volts */
/* Result in milliamps.  Value is approx 2.15 */
#define CURRENT_FACTOR  \
  (1000*VCC/(ADC_RESOLUTION*OUTPUT_RESISTANCE*SENSE_RESISTANCE*ZXCT_GM))
#define CURRENT_FACTOR_SCALE 8
#define SCALED_CURRENT_FACTOR \
  (uint16_t)(CURRENT_FACTOR_SCALE * CURRENT_FACTOR)

/* resistor divider to measure vbat */
#define UPPER_RESISTANCE 47000.0
#define LOWER_RESISTANCE 10000.0
/* Result in millivolts */
#define VOLTAGE_FACTOR \
((LOWER_RESISTANCE+UPPER_RESISTANCE)*VCC*1000/(LOWER_RESISTANCE*1024.0))
#define VOLTAGE_FACTOR_SCALE 2
#define SCALED_VOLTAGE_FACTOR (VOLTAGE_FACTOR_SCALE*VOLTAGE_FACTOR)

static uint16_t wait_adc(uint8_t mux)
{
  uint8_t i;

  PRR &= ~_BV(PRADC);                 /* Power on ADC */
  ADMUX = _BV(REFS0) | mux;
  ADCSRA = _BV(ADEN) | _BV(ADSC) | _BV(ADIF) | _BV(ADPS2) | _BV(ADPS1);
  /*
    Should complete in 25 cycles.  At 125kHz, this is 200us.
   */
  for (i = 215; i > 0; i--) {
    _delay_us(2);
    if (ADCSRA & _BV(ADIF)) break;
  }
  ADCSRA |= _BV(ADIF);                /* clear interrupt flag */
  PRR |= _BV(PRADC);                 /* Shut down ADC */
  return(ADC);
}

uint16_t bat_current(void)
{
  uint16_t lsb;

  lsb = wait_adc(MUX_IBAT);
  return (lsb * (uint16_t)(SCALED_CURRENT_FACTOR+0.5)) / CURRENT_FACTOR_SCALE;
}

uint16_t bat_voltage(void)
{
  uint16_t lsb;

  lsb = wait_adc(MUX_VBAT);
  return (lsb * (uint16_t)(SCALED_VOLTAGE_FACTOR+0.5))/VOLTAGE_FACTOR_SCALE;
}
