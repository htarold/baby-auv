#ifndef TX_H
#define TX_H
/*
  (C) 2019 Harold Tay LGPLv3
  Print to uart0, largely for diagnostics.
 */
#include <stdint.h>
#include <avr/pgmspace.h>

void tx_putc(char ch);
void tx_puts(char * s);
void tx_init(void);
void tx_putdec(int16_t d);
void tx_putdec32(int32_t d);
void tx_puthex(uint8_t x);
void tx_puthex32(uint32_t x);
void tx_msg(char * s, int16_t d);
void tx_putpgms(const char * s);
#define tx_strlit(lit) \
do { static const char s[] PROGMEM = lit; tx_putpgms(s); } while (0)
#define tx_bh() /* nothing */

#endif
