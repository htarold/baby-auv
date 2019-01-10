/* (C) 2019 Harold Tay LGPLv3 */
#include <stdint.h>
#include <string.h>
#include <avr/pgmspace.h>
#include "morse.h"
#include "file.h"
#include "syslog.h"
#include "f32.h"
#include "tx.h"

#define SYSLOG_WRITE_TO_DISK
#define DBG_SYSLOG

/*
  Compose a log message.
  Log messages can be streamed char by char instead of being
  written into a buffer first.  This approach works for the uart
  as well as the sd card, reduces use of an intermediate buffer.

  The first 3 uppercase  chars in a message are also
  sent out by morse code.
 */

#ifdef DBG_SYSLOG
#include "fmt.h"
#define PRINT(x) x
#define dbg_msg(a, b) tx_msg(a, b)
#define dbg_puts(a) tx_puts(a)
#else
#define PRINT(x)
#define dbg_msg(a, b) /* nothing */
#define dbg_puts(a) /* nothing */
#endif

#ifdef SYSLOG_WRITE_TO_DISK
#define LOGG(buf, len) logg(buf, len)
#else
#define LOGG(buf, len) 0
#endif

int8_t syslog_error = 0;

/*
  Serial output is optional because it takes a long time
  (not buffered XXX), and we may eventually want to transmit actual
  data, not diagnostics.
 */
static uint8_t serial_ok;
static struct file logfile;

int8_t syslog_init(const char * ident, uint8_t enable_serial_output)
{
#ifdef SYSLOG_WRITE_TO_DISK
  int8_t er;
  er = f32_init();
  if (er) return(er);
  tx_strlit("file_open...\r\n");
  er = file_open(&logfile, "LOGFILE TXT");
  if (er) return(er);
#endif
  serial_ok = enable_serial_output;
  morse_init();
  if (ident) {
    /* is actually in PROGMEM */
    syslog_putpgm(ident);
  }
  syslog_putlit(" Starting\n");
  return(0);
}

#define MAX_STAMP_SIZE sizeof("0x12345678.12 ")
static uint8_t maybe_stamp(char * bufp)
{
  static uint8_t uptimeh = 101;
  char * start;

  start = bufp;
  if (uptimeh != time_uptimeh) {
    uptimeh = time_uptimeh;
    *bufp++ = '\n';
    strcpy(bufp, fmt_32x(time_uptime));
    bufp += 10;
    *bufp++ = '.';
    strcpy(bufp, fmt_x(time_uptimeh));
    bufp += 2;
    *bufp++ = ' ';
  }
  return(bufp - start);
}
void syslog_put(char * msg, uint8_t len)
{
  static uint8_t breakable = 0;
  static int8_t morsed = 0;
  static char buf[32];                /* static to reduce stack */
  uint8_t used, i;
  int8_t res;

  for(i = used = 0; i < len; i++) {
    char ch;
    if (used >= sizeof(buf) - (MAX_STAMP_SIZE + 1)) {
#ifdef SYSLOG_WRITE_TO_DISK
      res = file_append(&logfile, buf, used);
      if (res) syslog_error = res;
#endif
      used = 0;
    }

    if (breakable) {
      if ((res = maybe_stamp(buf + used))) {
        used += res;
        morsed = 0;
        if (serial_ok) tx_puts("\r\n");
      }
    }

    ch = (msg[i] == '\n'?' ':msg[i]);
    buf[used++] = ch;

    /*
      Buzz out the first 3 uppercase chars in a record.
     */
    if (morsed < 3 && ch >= 'A' && ch <= 'Z') {
      morse_putc(ch);
      morsed++;
    }

    if (serial_ok) {
      if ('\n' == msg[i]) tx_putc('\r');
      tx_putc(msg[i]);
    }
    breakable = (ch == ' ');
  }

#ifdef SYSLOG_WRITE_TO_DISK
  res = file_append(&logfile, buf, used);
  if (res) syslog_error = res;
#endif
}
void syslog_puts(char * msg) { syslog_put(msg, strlen(msg)); }
void syslog_putpgm(const char * msg)
{
  for ( ; ; ) {
    char ch;
    ch = pgm_read_byte_near(msg);
    if (!ch) break;
    msg++;
    syslog_put(&ch, 1);
  }
}

void syslog_putc(char ch) { syslog_put(&ch, 1); }

void syslog_attrpgm(const char * name, int16_t val)
{
  syslog_put(" ", 1);
  syslog_putpgm(name);
  syslog_put("=", 1);
  syslog_i16d(val);
}
void syslog_lattrpgm(const char * name, int32_t val)
{
  syslog_put(" ", 1);
  syslog_putpgm(name);
  syslog_put("=", 1);
  syslog_i32d(val);
}

