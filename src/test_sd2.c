/* (C) 2019 Harold Tay GPLv3 */
#include "tx.h"
#include "sd2.h"
#include <util/delay.h>
#include <stdint.h>

void print_buffer(uint32_t addr)
{
  uint16_t i;

  tx_puts("Buffer at ");
  tx_puthex32(addr);
  tx_puts("\r\n");
  for(i = 0; i < sizeof(sd_buffer); i++) {
    if (0 == i%16) tx_puts("\r\n");
    else tx_putc(' ');
    tx_puthex(sd_buffer[i]);
  }
}

int main(void)
{
  uint16_t i;
  int8_t res;

  tx_init();
  for(i = 4; i > 0;i--) {
    tx_msg("Delay ", i);
    _delay_ms(1000);
  }

  res = sd_init();
  tx_msg("sd_init = ", res);
  tx_msg("sd_result = ", sd_result);
  tx_puts("sd_response = ");
  tx_puthex32(sd_response);
  tx_puts("\r\n");

  if (res) for( ; ; );

  for(i = 0; i < 4; i++) {
    tx_msg("Trying to read block ", i);
    res = sd_bread(i);
    if (0 == res) print_buffer(i);
    else tx_msg("Failed, res = ", res);
  }

  for( ; ; );

}


