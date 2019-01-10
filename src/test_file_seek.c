/* (C) 2019 Harold Tay GPLv3 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "tx.h"
#include "rx.h"
#include "sd2.h"
#include "file.h"
#include "handydefs.h"
#include "f32.h"
#include "fmt.h"

void panic(void)
{
  tx_puts("Stopping.\r\n");
  for( ; ; );
}

int
main(void)
{
  int8_t err, i;
  uint16_t count;
  uint16_t sizek;
  uint8_t start;
  char line[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz"
  "0123456789?\n"
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz"
  "0123456789?\n";
  struct file n;

  sei();
  tx_init();
  rx_init();
  rx_enable();

  for(i = 4; i > -1; i--){
    tx_msg("Delay ", i);
    _delay_ms(1000);
  }

  tx_puts("Calling sd_init\r\n");

  /*
    If SD card is power cycled, may need a second attempt to
    initialise.
   */
  err = f32_init();
  if (err)
    err = f32_init();
  if (err) {
    tx_msg("f32_init error: ", err);
    panic();
  }

  tx_msg("nr_sectors_per_cluster = ", f32_nr_sectors_per_cluster());

  tx_puts("Calling file_creatf\r\n");
  err = file_creatf(&n, "NEWFILE TXT");
  if (err) {
    tx_msg("file_creat: ", err);
    panic();
  }

  tx_puts("Initialised ok\r\n");

  /*
    Write random lengths of strings.
    (Random length is read from input, feed it from /dev/urandom).
    Quit after 1M is written.
    Logfile should have lines all nice and aligned.
   */

  count = 0;
  sizek = 0;

  for(start = 0; sizek < 1024; ){
    uint8_t len;
    while (!rx_havechar()) ;
    len = rx_getchar();
    len %= 64;
    file_append(&n, line + start, len);
    start += len;
    start %= 64;
    count += len;
    if (count >= 1024) {
      sizek++;
      count -= 1024;
      tx_msg("Written kbytes: ", sizek);
      tx_msg("       + bytes: ", count);
    }
  }

  /*
    Test seeking/writing.
    File is 1M in size.
   */
  for (count = 0; count < 100; count++) {
    uint32_t offset;
    uint16_t sector_offset;

    offset = 0;
    for (i = 0; i < 4; i++) {
      while (!rx_havechar()) ;
      offset <<= 8;
      offset |= rx_getchar();
    }
    offset &= 0x000fffff;  /* no larger than 1M-1 */
    err = file_seek(&n, offset);
    if (err) {
      tx_msg("file_seek returned ", err);
      panic();
    }
    sector_offset = offset % 512;
    sd_buffer[sector_offset] = ' ';
    tx_puts(fmt_32x(offset)); tx_puts("\r\n");
  }

  sd_buffer_sync();
  tx_puts("Done.\r\n");
  panic();
}
