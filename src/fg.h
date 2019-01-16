/*
  (C) 2019 Harold Tay LGPLv3
  Foreground task interface.
 */
#ifndef FG_H
#define FG_H

#include <stdint.h>

#define FG_STACK_BYTES 220            /* our stack size */

/*
  Getting date/time or position will not take longer than this.
  Default is 300s.
 */
extern void fg_set_gps_timeout(uint16_t tmo);
/*
  This task takes care of the 2 RF operations: GPS; and the
  radio modem (and later, the mission itself).
 */
extern void fg_task(void) __attribute__ ((noreturn));

/*
  User supplies this function.
  It should call odo_start() and odo_stop().
 */
extern void fg_mission(void);

/*
  A general purpose buffer is made available for use within the
  fg task.  Since modules within this task are independent, this
  buffer can be shared among them.
 */
#define FG_BUFFER_SIZE 64
extern uint8_t fg_buffer[FG_BUFFER_SIZE];

#endif /* FG_H */
