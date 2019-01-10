/* (C) 2019 Harold Tay GPLv3 */
/*
  Show contents of eeprom, and maybe change it.
 */

#include <avr/interrupt.h>
#include "ee.h"
#include "time.h"
#include "tx.h"

void display(char * name, void * p, uint8_t size)
{
  uint16_t value;
  if (1 == size) {
    uint8_t u;
    ee_load(&u, (uint8_t *)p, 1);
    value = u;
  } else if (2 == size) {
    ee_load(&value, (uint16_t *)p, 2);
  }

  tx_puts(name);
  tx_puts("=");
  tx_putdec(value);
  tx_puts("\r\n");
}

#define S(a) a, sizeof(*(a))
int
main(void)
{
  int8_t i;

  tx_init();
  time_init();
  sei();
  for(i = 0; i < 3; i++) {
    tx_msg("Starting ", 3 - i);
    time_delay(100);
  }

  display("tude.accel_offsets[0]", S(ee.tude.accel_offsets + 0));
  display("tude.accel_offsets[1]", S(ee.tude.accel_offsets + 1));
  display("tude.accel_offsets[2]", S(ee.tude.accel_offsets + 2));
  display("tude.cmpas_offsets[0]", S(ee.tude.cmpas_offsets + 0));
  display("tude.cmpas_offsets[1]", S(ee.tude.cmpas_offsets + 1));
  display("tude.cmpas_offsets[2]", S(ee.tude.cmpas_offsets + 2));
  display("depth.depth_gain1024", S(&ee.depth.depth_gain1024));
  display("depth.depth_offset", S(&ee.depth.depth_offset));
  display("servo.reversed", S(&ee.servo.reversed));
  display("servo.centre", S(&ee.servo.centre));
  display("servo.throw", S(&ee.servo.throw));
  display("pitch.act1", S(&ee.pitch.act1));
  display("pitch.act2", S(&ee.pitch.act2));
  display("twirl.offset", S(&ee.twirl.offset));

  for( ; ; );
}
