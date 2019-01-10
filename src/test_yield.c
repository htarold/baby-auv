/* (C) 2019 Harold Tay GPLv3 */
/*
  Cooperative multitasking?
 */

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "tx.h"
#include "yield.h"

void task_0(void)
{
  uint16_t count;
  yield();
  for (count = 0; ; count++) {
    tx_puts("Task 0, count = ");
    tx_putdec(count);
    tx_puts("\r\n");
    yield();
    _delay_ms(500);
  }
}
void task_1(void)
{
  uint16_t count;
  yield();
  for (count = 100; ; count++) {
    tx_puts("Task 1, count = ");
    tx_putdec(count);
    tx_puts("\r\n");
    yield();
    _delay_ms(500);
  }
}
void task_2(void)
{
  uint16_t count;
  yield();
  for (count = 200; ; count++) {
    tx_puts("Task 2, count = ");
    tx_putdec(count);
    tx_puts("\r\n");
    yield();
    _delay_ms(500);
  }
}

/*
  Thread entry functions have return type void but must not ever
  return.  The function calls yield() to yield the processor.
  tid is the thread id, the index of the entry in the tasks[] array.
 */

struct task tasks[] = {
/* Fill in thread entry functions here.  Leave sp empty */
  { task_0, 0, },
  { task_1, 0, },
  { task_2, 0, },
};

int
main(void)
{
  uint8_t i;
  /*
    SP is assumed to be RAMEND or close to it.
   */
  sei();
  tx_init();

  yield_init();

  for (i = 3; i > 0; i--) {
    tx_puts("Delay ");
    tx_putdec(i);
    tx_puts("\r\n");
    _delay_ms(1000);
  }
  for (i = 0; i < sizeof(tasks)/sizeof(*tasks); i++) {
    tx_puts("Task_"); tx_putdec(i); tx_puts(" address = 0x");
    tx_puthex((uint16_t)tasks[i].pc >> 8);
    tx_puthex((uint16_t)tasks[i].pc&0xff);
    tx_puts(", Stack = 0x");
    tx_puthex((uint16_t)tasks[i].sp >> 8);
    tx_puthex((uint16_t)tasks[i].sp&0xff);
    tx_puts("\r\n");
  }
  task_0();
  /* not reached */
}
