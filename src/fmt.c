/* (C) 2019 Harold Tay LGPLv3 */
#include <stdint.h>
#include <avr/pgmspace.h>
#include "fmt.h"

char fmt_buf[12];

#ifdef FMT_DEBUG
static void init_buf(void)
{
  uint8_t i;
  for(i = 0; i < sizeof(fmt_buf)-1; i++)
    fmt_buf[i] = '?';
  fmt_buf[sizeof(fmt_buf)-1] = '\0';
}
#else
#define init_buf() fmt_buf[sizeof(fmt_buf)-1] = '\0'
#endif
char * fmt_u32d(uint32_t d)
{
  uint8_t i;
  init_buf();
  i = sizeof(fmt_buf) - 1;
  do fmt_buf[--i] = '0' + (d%10); while( (d /= 10) );
  return(fmt_buf + i);
}

char * fmt_i32d(int32_t d)
{
  if( d < 0 ){
    char * p;
    p = fmt_u32d(0 - d);
    *(--p) = '-';
    return(p);
  }else
    return(fmt_u32d(d));
}

char * fmt_u16d(uint16_t d)
{
  uint8_t i;
  init_buf();
  i = sizeof(fmt_buf)-1;
  do fmt_buf[--i] = '0' + (d%10); while( (d /= 10) );
  return(fmt_buf + i);
}
char * fmt_i16d(int16_t d)
{
  if( d < 0 ){
    char * p;
    p = fmt_u16d(0 - d);
    *(--p) = '-';
    return(p);
  }else
    return(fmt_u16d(d));
}

static inline char fmt_n(uint8_t nyb)
{
  nyb &= 0xf;
  return(nyb<10?'0'+nyb:'a'-10+nyb);
}
char * fmt_x(uint8_t d)
{
  uint8_t i;
  init_buf();
  i = sizeof(fmt_buf) - 1;
  fmt_buf[--i] = fmt_n(d);
  d >>= 4;
  fmt_buf[--i] = fmt_n(d);
  return(fmt_buf+i);
}
char * fmt_32x(uint32_t x)
{
  uint8_t i;
  char * bufp;
  static const char PROGMEM hexchars[] = "0123456789abcdef";
  init_buf();
  bufp = fmt_buf;
  *bufp++ = '0';
  *bufp++ = 'x';
  for(i = 0; i < 8; i++){
    *bufp++ = pgm_read_byte_near(hexchars + ((x & 0xf0000000)>>28));
    x <<= 4;
  }
  *bufp = '\0';
  return(fmt_buf);
}

