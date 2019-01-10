/* (C) 2019 Harold Tay LGPLv3 */
#ifndef SYSLOG_H
#define SYSLOG_H
#include <stdint.h>
#include <avr/pgmspace.h>

/*
  End all records with \n.
  Can call syslog_init() again to disable/enable serial output,
  but only between records.

  Wut?
 */

/*
  syslog functions return void because there is no way to handle
  failure at this level.  However syslog_error will be set, so
  can be checked at a higher level periodically.
 */
extern int8_t syslog_error;

/*
  ident is in PROGMEM
  Must be called before any other syslog functions.
 */
#define SYSLOG_WRITE_TO_SD
#define SYSLOG_DO_COPY_TO_SERIAL 1
#define SYSLOG_DONT_COPY_TO_SERIAL 0
extern int8_t syslog_init(const char * ident, uint8_t copy_to_serial);

/*
  These are most often used
 */
extern void syslog_attrpgm(const char * name, int16_t val);
extern void syslog_lattrpgm(const char * name, int32_t val);
#define syslog_attr(lit, v) \
do { static const char s[] PROGMEM = lit; syslog_attrpgm(s, v); } while (0)
#define syslog_lattr(lit, v) \
do { static const char s[] PROGMEM = lit; syslog_lattrpgm(s, v); } while (0)

/*
  The following are not used much
 */
extern void syslog_putpgm(const char * s);
#define syslog_putlit(lit) \
{ static const char s[] PROGMEM = lit; syslog_putpgm(s); }
extern void syslog_putc(char ch);
extern void syslog_puts(char * msg);
extern void syslog_put(char * msg, uint8_t len);

#include "fmt.h"
#define syslog_u32d(d) syslog_puts(fmt_u32d(d))
#define syslog_i32d(d) syslog_puts(fmt_i32d(d))
#define syslog_u16d(d) syslog_puts(fmt_u16d(d))
#define syslog_i16d(d) syslog_puts(fmt_i16d(d))
#define syslog_x(d)    syslog_puts(fmt_x(d))
#define syslog_32x(d)  syslog_puts(fmt_32x(d))

#endif /* SYSLOG_H */
