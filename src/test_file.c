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
#include "sd2.h"

/*
  Using Toshiba 8GB micro SD, it takes 700s to complete both
  files to 1MB, regardless of whether sd2 is clocked at 250kHz
  or 4MHz.  Current consumption including main board is about
  14mA at 14.1V (200mW).  sd2 has a single sector buffer.
 */

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
  char * newfile;
  char line[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz"
  "0123456789?\n"
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz"
  "0123456789?\n";
  static struct file f, n;

  newfile = "NEWFILE TXT";
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

  tx_puts("Calling file_openf\r\n");
  err = file_openf(&f, "LOGFILE TXT");
  if (err) {
    tx_msg("file_open: ", err);
    panic();
  }

  tx_puts("Initialised ok\r\n");

  err = f32_mkdir("NEWD       ");
  if (err) {
    tx_msg("f32_mkdir(NEWD):", err);
    panic();
  }
  tx_puts("mkdir(NEWD) ok\r\n");

  err = f32_chdir("NEWD       ");
  if (err) {
    tx_msg("f32_chdir(NEWD):", err);
    panic();
  }
  tx_puts("chdir(NEWD) ok\r\n");

  tx_puts("Calling file_creatf\r\n");
  err = file_creatf(&n, newfile);
  if (err) {
    tx_msg("file_creat: ", err);
    panic();
  }
  tx_puts("file_creat(NEWFILE TXT) ok\r\n");

  err = f32_chdir(0);
  if (err) {
    tx_msg("f32_chdir(0):", err);
    panic();
  }
  tx_puts("chdir(0) ok\r\n");

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
    file_append(&f, line + start, len);
    file_append(&n, line + start, len);
    start += len;
    start %= 64;
    count += len;
    if (count >= 1024) {
      sizek++;
      count -= 1024;
      tx_msg("\r\nWritten kbytes: ", sizek);
      tx_msg("       + bytes: ", count);
/*
#define MSG32(str, u32) tx_puts(str); tx_puthex32(u32);
      MSG32("n.f.file_size = ", n.f.file_size);
      MSG32("\r\nn.f.file_1st_cluster = ", n.f.file_1st_cluster);
 */
    }
  }
  /* needed? newfile.txt does not get last sector written */
  sd_buffer_sync();
  tx_puts("Done.\r\n");
  panic();
}
