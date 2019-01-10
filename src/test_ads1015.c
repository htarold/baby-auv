/* (C) 2019 Harold Tay GPLv3 */
#include <stdint.h>
#include <util/delay.h>
#include "tx.h"
#include "i2c.h"
#include "ads1015.h"

int
main(void)
{
  int8_t i, er;
  uint16_t cfg;
  int16_t code;

  tx_init();
  i2c_init();

  for(i = 3; i > 0; i--) {
    _delay_ms(1000);
    tx_msg("Delay ", i);
  }

  cfg = ADS1015_CFG_OS             /* begin conversion */
      | ADS1015_MUX_AIN0AIN1
      | ADS1015_PGA_4096MV
      | ADS1015_MODE_1SHOT
      | ADS1015_SPS_3K3;

  for( ; ; ) {
    er = ads1015_setup(ADS1015_ADDR_GND, cfg);
    if (er) {
      tx_msg("Setup failed: ", er);
      for( ; ; );
    }
    tx_puts("Setup ok\r\n");

    er = ads1015_get(ADS1015_ADDR_GND, &code);
    tx_msg("er: ", er);
    tx_msg("Code: ", code);
    _delay_ms(1000);
  }

}
