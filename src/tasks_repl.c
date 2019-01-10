/* (C) 2019 Harold Tay GPLv3 */
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "time.h"
#include "syslog.h"
#include "yield.h"
#include "repl.h"

void fg_mission(void)
{
  int8_t er;
  struct waypoint w;


  /*
    Check mission script.
   */
  er = repl_init();
  if (er) panic("Err:Repl_Init", er);
  for ( ; ; ) {
    yield();
    er = repl_next(&w);
    syslog_attr("repl_next", er);
    if (er < 0)
      panic("Fatal:Repl_Error", er);
    if (er == REPL_END) break;
  }

  syslog_attr("repl_scanned", 0);

  /*
    This is where we would actually execute the mission.
   */
  er = repl_init();
  if (er) panic("Err:Repl_Init", er);
  for ( ; ; ) {
    yield();
    er = repl_next(&w);
    syslog_attr("repl_next", er);
    if (er == REPL_END) break;
    if (w.n.lat && w.n.lon) {
      syslog_lattr("w_lat", w.n.lat);
      syslog_lattr("w_lon", w.n.lon);
      syslog_attr("w_depth", w.depth_cm);
      syslog_lattr("w_until", w.until);
      syslog_lattr("w_status_offset", w.status_offset);
    }
    er = repl_done(&w, REPL_DONE_OK);
    syslog_attr("repl_done", er);
  }
}
