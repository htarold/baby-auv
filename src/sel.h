/* (C) 2019 Harold Tay LGPLv3 */
#ifndef SEL_H
#define SEL_H
#include <stdint.h>
/*
  Selector macros for GPS and RF.  Bits refer to the pcf8574
  port expander, which is set to all outputs.
 */

#define BIT_UNUSED_0   0x01
#define BIT_nPOWER_GPS 0x02
#define BIT_nPOWER_RF  0x04
#define BIT_UNUSED_3   0x08
#define BIT_SERIAL_RF  0x10 /* Serial RF connects both Tx and Rx */
#define BIT_SERIAL_GPS 0x20 /* Serial GPS connects GPS Tx, not Rx */
#define BIT_nRF_SET    0x40
#define BIT_UNUSED_7   0x80

extern uint8_t pcf8574_port;
#define ALL_BITS_DEFAULT \
pcf8574_port = (BIT_nPOWER_RF | BIT_nPOWER_GPS | BIT_nRF_SET)

#define RF_POWER_ON    pcf8574_port &= ~(BIT_nPOWER_RF)
#define RF_POWER_OFF   pcf8574_port |= BIT_nPOWER_RF
#define RF_SERIAL_ON   pcf8574_port |= BIT_SERIAL_RF
#define RF_SERIAL_OFF  pcf8574_port &= ~(BIT_SERIAL_RF)

#define GPS_POWER_ON   pcf8574_port &= ~(BIT_nPOWER_GPS)
#define GPS_POWER_OFF  pcf8574_port |= BIT_nPOWER_GPS
#define GPS_SERIAL_ON  pcf8574_port |= BIT_SERIAL_GPS
#define GPS_SERIAL_OFF pcf8574_port &= ~(BIT_SERIAL_GPS)

/*
  Legacy deprecated use of selector, retains old semantics.
 */

#define PCF_WRITE pcf_write(addr, pcf8574_port)
#define SEL_RF_POWER_ON \
(RF_POWER_ON, RF_SERIAL_ON, GPS_POWER_OFF, GPS_SERIAL_OFF, sel_write())

#define SEL_RF_POWER_OFF  \
(ALL_BITS_DEFAULT, sel_write())

#define SEL_GPS_POWER_ON \
(GPS_POWER_ON, GPS_SERIAL_ON, RF_POWER_OFF, RF_SERIAL_OFF, sel_write())

#define SEL_GPS_POWER_OFF \
(ALL_BITS_DEFAULT, sel_write())

/*
  New interface.
 */

/* Serial set to RF and power RF on (GPS power can remain on) */
#define SEL_RF_ON \
((RF_POWER_ON), (RF_SERIAL_ON), (GPS_SERIAL_OFF), sel_write())

/* RF serial and RF power off */
#define SEL_RF_OFF \
((RF_POWER_OFF), (RF_SERIAL_OFF), sel_write())

/* Serial set to GPS and power GPS on (RF power can remain on) */
#define SEL_GPS_ON \
((GPS_POWER_ON), (GPS_SERIAL_ON), (RF_SERIAL_OFF), sel_write())

/* GPS serial and GPS power off */
#define SEL_GPS_OFF \
((GPS_POWER_OFF), (GPS_SERIAL_OFF), sel_write())

#define SEL_INIT \
(ALL_BITS_DEFAULT, sel_write())

/* Can return I2C errors */
int8_t sel_write(void);
int8_t sel_init(void);

#endif /* SEL_H */
