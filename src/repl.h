/* (C) 2019 Harold Tay LGPLv3 */
#ifndef REPL_H
#define REPL_H
#include "nav.h"

/*
  The mission script is found at the start of LOGFILE.TXT.
  (normal system logging is appended to this same file, and does
  not interfere with the mission script).
  The script consists of directives and comments.
  Lines starting with '#' (at column 1) are comments.
  Blank lines not allowed.

  Waypoint directive:
  "$waypoint,"lat,[N|S],lon,[E|W],rate,[depth_cm],[date],[time]"\n"
  On completion of a waypoint, '$' is changed to '+' if
  successful, or '!' if not.
  lat and lon are just like for GPS:
  lat: ddmm.ffff[f...]
  lon: dddmm.ffff[f...]
  where dd and ddd are 2 or 3 digits of degrees (0-padded on the
  left); mm.ffff[f...] are the minutes (0-padded) and at least 4
  decimal places.  N/S or E/W are required to indicate positive or
  negative (S and W are negative).  Comment lines start with '#'
  in the first column.

  Auv ID directive:
  "$auvid,"number where 0<number<=255
  This can only appear once.

  End directive:
  "$end,"lat,[N|S],lon,[E|W],rate,[depth_cm]"\n"
  Stops processing more directives at this point.  Typically the
  log file starts after this point.
 */

struct waypoint {
  struct nav_pt n;                    /* where */
  uint32_t until;                     /* keep station until this time */
  uint32_t status_offset;
  int16_t sample_rate;
  int16_t depth_cm;                   /* Desired cruising depth */
};

#define REPL_WAYPOINT            0
#define REPL_END                 1
#define REPL_AUVID               2
#define REPL_ERR_UNRECOGNISED    -11  /* unrecognised directive */
#define REPL_ERR_CONVERSION      -12  /* failed numeric conversion */
#define REPL_ERR_DATETIMESPEC    -13  /* Cannot convert date/time */
#define REPL_ERR_SYNTAX          -14  /* General syntax error */
#define REPL_ERR_BADSTATUS       -15  /* Bad status char */
#define REPL_ERR_BADID           -16  /* Bad auv id */
#define REPL_ERR_DUPID           -17  /* Duplicate auv id */
#define REPL_ERR_JUNK            -18  /* trailing junk */

extern int8_t repl_init(void);

/*
  Get the next directive.  Returns REPL_WAYPOINT or REPL_END or
  REPL_AUVID, or error.
 */
extern int8_t repl_next(struct waypoint * wp);

/*
  Return current line number.
 */
extern uint16_t repl_line_number(void);

/*
  Mark it as done so the directive will not be visible if scanning
  is restarted.
 */
#define REPL_DONE_OK '+'
#define REPL_DONE_FAILED '!'
extern int8_t repl_done(struct waypoint * wp, char repl_status);

/*
  Returns assigned AUV ID, or 0 if not assigned.
 */
extern uint16_t repl_get_id(void);

#endif
