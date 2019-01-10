/* (C) 2019 Harold Tay LGPLv3 */
/*
  Cooperative multitasking scheduler.  All tasks are declared in
  tasks.h, and main() will run them in turn.  Tasks should not
  poll or spin-wait; instead, they should call yield().
 */

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <string.h>
#include "tx.h"
#include "i2c.h"
#include "yield.h"
#include "syslog.h"
#include "time.h"
#include "bg.h"
#include "fg.h"
#include "morse.h"

#define FILL_BYTE 0xa                 /* Define to test stack size */
#ifdef FILL_BYTE
#undef  DUMP_STACKS                   /* Dump stacks periodically */
#endif

#undef PROFILE                        /* PC3 goes high when asleep */
#ifdef PROFILE
#define PROFILE_PIN_INIT DDRC |= _BV(PC3)
#define PROFILE_PIN_HI PORTC |= _BV(PC3)
#define PROFILE_PIN_LO PORTC &= ~_BV(PC3)
#else
#define PROFILE_PIN_INIT /* nothing */
#define PROFILE_PIN_HI   /* nothing */
#define PROFILE_PIN_LO   /* nothing */
#endif

static struct task {
  void (*pc)(void);
  union { uint16_t size; void * ptr; } stack;
} tasks[3] = {
  { .pc = 0, .stack.size = 128, },    /* main() task (us) */
  { .pc = bg_task, .stack.size = BG_STACK_BYTES, },
  { .pc = fg_task, .stack.size = FG_STACK_BYTES, },
};

#define NR_TASKS (sizeof(tasks)/sizeof(*tasks))

static uint8_t * stack_end[NR_TASKS];

#undef DEBUG
#ifdef DEBUG
#define DBG(x) x
#else
#define DBG(x) /* nothing */
#endif

#define RETURN_HBYTE 1+19
#define RETURN_LBYTE 2+19

static void putaddr(uint16_t x) { tx_puthex(x>>8); tx_puthex(x&0xff); }
static void printtask(uint8_t i)
{
  uint8_t * p;
  tx_strlit("tasks[");
  tx_putdec(i);
  tx_strlit("].pc = ");
  putaddr((uint16_t)(tasks[i].pc)*2);
  tx_strlit(" .stack.ptr = ");
  putaddr((uint16_t)tasks[i].stack.ptr);
  tx_strlit(" returns to: ");
  p = tasks[i].stack.ptr;
  tx_puthex(p[RETURN_HBYTE]);
  tx_puthex(p[RETURN_LBYTE]);
  tx_strlit(" stack_end = ");
  putaddr((uint16_t)stack_end[i]);
  tx_puts("\r\n");
}

static struct task * current;

/*
  RETURN_HBYTE and RETURN_LBYTE are the stack offsets for the
  return address, and will change depending on how many
  registers yield() will push.
 */

#ifdef DEBUG
static uint16_t return_address(uint8_t * sp)
{
  uint16_t pc;
  pc = sp[RETURN_LBYTE];
  pc |= sp[RETURN_HBYTE] << 8;
  return(pc);
}
#endif

void yield(void)
{
  sei();                              /* All MUST have interrupts on */
  __asm__ __volatile__(
  "in r0,__SREG__" "\n\t"
  "push r0" "\n\t"
  "push r2" "\n\t"
  "push r3" "\n\t"
  "push r4" "\n\t"
  "push r5" "\n\t"
  "push r6" "\n\t"
  "push r7" "\n\t"
  "push r8" "\n\t"
  "push r9" "\n\t"
  "push r10" "\n\t"
  "push r11" "\n\t"
  "push r12" "\n\t"
  "push r13" "\n\t"
  "push r14" "\n\t"
  "push r15" "\n\t"
  "push r16" "\n\t"
  "push r17" "\n\t"

  "push r28" "\n\t"
  "push r29" "\n\t"
  ::);

  /* Save caller's stack */
  current->stack.ptr = (void *)SP;
  DBG(current->pc = (void *)return_address((uint8_t *)SP));
  DBG(printtask(current - tasks));

  current++;
  if (current >= tasks + NR_TASKS) current = tasks;

  /* Restore callee's stack */
  cli();
  SP = (uint16_t)current->stack.ptr;
  sei();
  DBG(current->pc = (void *)return_address((uint8_t *)SP));
  DBG(printtask(current - tasks));

  __asm__ __volatile__(
  "pop r29" "\n\t"
  "pop r28" "\n\t"

  "pop r17" "\n\t"
  "pop r16" "\n\t"
  "pop r15" "\n\t"
  "pop r14" "\n\t"
  "pop r13" "\n\t"
  "pop r12" "\n\t"
  "pop r11" "\n\t"
  "pop r10" "\n\t"
  "pop r9" "\n\t"
  "pop r8" "\n\t"
  "pop r7" "\n\t"
  "pop r6" "\n\t"
  "pop r5" "\n\t"
  "pop r4" "\n\t"
  "pop r3" "\n\t"
  "pop r2" "\n\t"
  "pop r0" "\n\t"
  "out __SREG__,r0" "\n\t"
  ::);
  /* "return" to the next task. */
}

void ydelay(uint8_t nr_yields)
{
  for ( ; nr_yields > 0; nr_yields--) yield();
}

/*
  The allocated stack pointers are NOT the tasks stack bottom!
  The actual stack bottom is about 19 bytes off.  This is because
  the first call to yield() will pop rubbish into the regs, and
  the SP will change.  This is significant for setting the
  stack_end[] extents.
 */
static void prepare_stack(void)
{
  int8_t i;
  uint8_t * sp;
  extern uint8_t __bss_end;
  int16_t ram_left;
  uint16_t stack_size;

  sp = (uint8_t *)SP - 2;
  ram_left = sp - &__bss_end;

#ifdef FILL_BYTE
  memset(&__bss_end, FILL_BYTE, ram_left);
#endif

  /*
    Task[0] is handled differently ...
   */
  stack_size = tasks[0].stack.size;
  tasks[0].stack.ptr = sp; /* not used or useful */
  sp -= stack_size;
  stack_end[0] = sp;
  printtask(0);

  /*
    ... than other tasks, for which yield() needs to make
    the first call.
   */
  for (i = 1; i < NR_TASKS; i++) {
    int8_t j;

    *sp-- = (uint16_t)tasks[i].pc & 0xff;
    *sp-- = ((uint16_t)tasks[i].pc)>>8;
    for (j = 0; j < 19; j++)          /* XXX upon first call, */
      *sp-- = 0;                      /* task has empty regs */
    stack_size = tasks[i].stack.size;
    tasks[i].stack.ptr = sp;
    sp -= stack_size;
    stack_end[i] = sp;                /* See note above */
    printtask(i);
    ram_left = sp - (uint8_t *)&__bss_end;
    if (ram_left < 0) {
      for ( ; ; ) {
        tx_strlit("FATAL: too little RAM! ");
        _delay_ms(500);
      }
    }
  }
  tx_putdec(NR_TASKS);
  tx_strlit(" tasks, ");
  tx_putdec(ram_left);
  tx_strlit(" bytes RAM unused.\r\n");
  syslog_attr("ram_unused", ram_left);
}

static void countdown(void)
{
  int8_t i;
  for (i = 3; i > 0; i--) {
    tx_strlit("Starting up, delay ");
    tx_putdec(i);
    tx_puts("\r\n");
    _delay_ms(1000);
  }
}

#ifdef FILL_BYTE

static void check_stacks(void)
{
  int8_t i;
  for (i = 0; i < NR_TASKS; i++) {
    static const char prefix[] PROGMEM = " stack_free_";
    uint8_t * p;
    char ch;
    for (p = stack_end[i]+1; FILL_BYTE == *p; p++);
    syslog_putpgm(prefix);
    ch = '0' + i; syslog_put(&ch, 1);
    ch = '=';     syslog_put(&ch, 1);
    syslog_u16d(p - (stack_end[i]+1));
  }
}
#ifdef DUMP_STACKS
static void dump_stacks(void)
{
  extern uint8_t __bss_end;
  uint8_t * addr;
  uint8_t i;

  for (i = 0; i < NR_TASKS; i++)
    printtask(i);

  i = 0;
  for (addr = (void *)RAMEND; addr > &__bss_end; addr--, i++) {
    if (0 == (i & 15)) {
      tx_puts("\r\n");
      tx_puthex((uint16_t)addr>>8);
      tx_puthex((uint16_t)addr&0xff);
      tx_puts(":");
      i = 0;
    } else
      tx_putc(' ');
    tx_puthex(*addr);
  }
}
#else
#define dump_stacks() /* nothing */
#endif
#else
#define check_stacks() /* nothing */
#endif

int
main(void)
{
  uint8_t h;
  uint8_t flag_checked;
  int8_t er;
#ifndef IDENT
#define IDENT ident
#endif
#define STR(x) #x
#define DECLARE_IDENT(x) static const char ident[] PROGMEM = STR(x)
  DECLARE_IDENT(IDENT);
  uint8_t mcusr = MCUSR;
  MCUSR = 0;

  tx_init();
  countdown();
  sei();
  time_init();
  i2c_init();
  er = syslog_init(ident, SYSLOG_DO_COPY_TO_SERIAL);
  while (er) {
    tx_msg("FATAL:syslog_init = ", er);
    _delay_ms(1000);
  }

  /* How much time spent busy/sleeping? */
  PROFILE_PIN_INIT;

  syslog_attr("mcusr", mcusr);

  /*
    Turn off analogue comparator (not used).
   */
  ACSR |= _BV(ACD);

  prepare_stack();

  /*
    We are task[0];
   */
  current = tasks;
  flag_checked = 0;

  for ( ; ; ) {
#if 1
    if (0 == (time_uptime & (1024-1))) {
      dump_stacks();
    }
#endif
    /*
      Only check if there's plenty of time.
     */
#if 1
#define STACK_CHECK_MASK 31
    if (0 == (time_lsb & STACK_CHECK_MASK)) { /* XXX */
      if (! flag_checked
        && TCNT1 < (3*(TIMER_TOP/4))) {       /* low cpu load */
        check_stacks();
	dump_stacks();
      }
      flag_checked = 1;
    } else if (1 == (time_lsb & STACK_CHECK_MASK)) {
      flag_checked = 0;
    }
#endif
    h = time_100s;
    yield();
    /*
      If we come back before the next tick, sleep until the next
      tick.  Otherwise, high processor load, so don't sleep.
     */
    while (h == time_100s) {
      set_sleep_mode(SLEEP_MODE_IDLE);
      PROFILE_PIN_HI;
      sleep_mode();
      PROFILE_PIN_LO;
    }
    morse_10ms();
  }
  /* Not Reached */
}

void panic_pgm(const char * msg, int16_t val)
{
  for ( ; ; ) {
    syslog_attrpgm(msg, val);
    ydelay(100);
    ydelay(100);
    ydelay(100);
    ydelay(100);
    ydelay(100);
  }
}
