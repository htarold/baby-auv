/* (C) 2019 Harold Tay GPLv3 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "buzz.h"
#include "tx.h"

int
main(void)
{
  tx_init();
  sei();
  for( ; ; ) {
    buzz(DIDA); _delay_ms(500);
    buzz(DADIDIDI); _delay_ms(500);
    buzz(DADIDADI); _delay_ms(500);
    buzz(DADIDI); _delay_ms(500);
    buzz(DI); _delay_ms(500);
    buzz(DIDIDADI); _delay_ms(500);
    buzz(DADADI); _delay_ms(500);
    buzz(DIDIDIDI); _delay_ms(500);
    buzz(DIDI); _delay_ms(500);
    buzz(DIDADADA); _delay_ms(500);
    buzz(DADIDA); _delay_ms(500);
    buzz(DIDADIDI); _delay_ms(500);
    buzz(DADA); _delay_ms(500);
    buzz(DADI); _delay_ms(500);
    buzz(DADADA); _delay_ms(500);
    buzz(DIDADADI); _delay_ms(500);
    buzz(DADADIDA); _delay_ms(500);
    buzz(DIDADI); _delay_ms(500);
    buzz(DIDIDI); _delay_ms(500);
    buzz(DA); _delay_ms(500);
    buzz(DIDIDA); _delay_ms(500);
    buzz(DIDIDIDA); _delay_ms(500);
    buzz(DIDADA); _delay_ms(500);
    buzz(DADIDIDA); _delay_ms(500);
    buzz(DADIDADA); _delay_ms(500);
    buzz(DADADIDI); _delay_ms(500);
  }
}
