/* (C) 2019 Harold Tay LGPLv3 */
/*
  Mission script interpreter.
 */

#include <avr/pgmspace.h>
#include <string.h>
#include "csv.h"
#include "cal.h"
#include "fg.h"                       /* for fg_buffer[] */
#include "file.h"
#include "sd2.h"
#include "syslog.h"
#include "repl.h"
#include "nav.h"                     /* for NAV_MAKE_NULL */

#undef DEBUG
#ifdef DEBUG
#include "tx.h"
#define DBG(x) x
#else
#define DBG(x) /* nothing */
#endif

#define scan_buffer ((char *)fg_buffer)

static struct file script;
static uint32_t seek;
#define SECTOR(sk) (sk>>10)
#define OFFSET(sk) (sk & ((1UL<<9)-1))

static uint16_t repl_auvid;
static uint16_t nr_waypoints;
static uint16_t line_nr;

uint16_t repl_line_number(void) { return(line_nr); }
int8_t repl_init(void)
{
  seek = 0UL;
  nr_waypoints = line_nr = 0;
  repl_auvid = 0;
  return(file_open(&script, "LOGFILE TXT"));
}

uint16_t repl_get_id(void) { return(repl_auvid); }

static int8_t get_line(void)
{
  uint8_t i;
  int8_t er;

  /* Refresh the sd_buffer */
  DBG(tx_msg("Seeking to ", (int16_t)seek));
  er = file_seek(&script, seek);
  if (er) goto seek_error;

  for (i = 0; seek <= script.f.file_size; ) {
    char ch;
    if (!OFFSET(seek)) {
      DBG(tx_msg("Seeking to ", (int16_t)seek));
      er = file_seek(&script, seek);
      if (er) goto seek_error;
    }
    ch = (char)(sd_buffer[OFFSET(seek)]);
    seek++;
    if ('\r' == ch) continue;
    scan_buffer[i] = ch;
    if (i < FG_BUFFER_SIZE-2) i++;
    if ('\n' == ch) break;
  }
  line_nr++;
  scan_buffer[i] = '\0';
  DBG(tx_puts(">>"));
  DBG(tx_puts(scan_buffer));
  DBG(tx_puts("<<\r\n"));
  return(0);
seek_error:
  syslog_attr("Err:Repl_Seek", er);
  syslog_attr("repl_line_nr", line_nr);
  return(er);
}

/* Returns 0 on success, 1 if end of file, <0 inherited error */
static int8_t current_directive;
int8_t repl_next(struct waypoint * wp)
{
  uint8_t tm[3], dt[3], c;
  uint8_t have_time, have_date, flag_end;
  char * p;

  flag_end = 0;
  memset(wp, 0, sizeof(*wp));

  for ( ; ; ) {
    int8_t er;
    wp->status_offset = seek;
    er = get_line();
    if (er) {
      return(current_directive = er);
    }
    if ('#' == *scan_buffer) continue;
    if ('!' == *scan_buffer) continue;
    if ('+' == *scan_buffer) continue;
    if ('$' == *scan_buffer) break;
    goto unknown;
  }

#define STRCMP(lit) \
({ static const char s[] PROGMEM = lit; \
   strncmp_P(scan_buffer, s, sizeof(s)-1); })

  if (0 == STRCMP("$auvid,")) {
    p = scan_buffer + sizeof("$auvid,")-1;
    csv_accum = 0;
    c = csv_numeric(p, 6);
    if (!c) {
      syslog_attr("Err:Repl_Convert_at", line_nr);
      return(current_directive = REPL_ERR_CONVERSION);
    }
    if (csv_accum < 1 || csv_accum > 65535) {
      syslog_attr("Err:Repl_bad_Auvid_at", line_nr);
      return(current_directive = REPL_ERR_BADID);
    }
    if (repl_auvid && repl_auvid != csv_accum) {
      syslog_attr("Err:Repl_Auvid_again_at", line_nr);
      return(current_directive = REPL_ERR_DUPID);
    }
    repl_auvid = csv_accum;
    return(current_directive = REPL_AUVID);
  }

  if (0 == STRCMP("$end,")) {
    flag_end = 1;
    p = scan_buffer + sizeof("$end,")-1;
  } else if (0 == STRCMP("$waypoint,")) {
    flag_end = 0;
    p = scan_buffer + sizeof("$waypoint,")-1;
  } else { unknown:
    syslog_attr("Err:Repl_Unknown_cmd_at", line_nr);
    return(current_directive = REPL_ERR_UNRECOGNISED);
  }

  c = csv_latlon(p, &(wp->n));
  if (!c) {
    syslog_attr("Err:Repl_Latlon_conversn_at", line_nr);
    return(current_directive = REPL_ERR_CONVERSION);
  }

  if (c <= sizeof(",,,,")-1) {      /* empty fields */
    NAV_MAKE_NULL(wp->n);
  }
  p += c;

  DBG(tx_msg("##line_nr=", line_nr));
  DBG(tx_puts("## lat:"));
  DBG(tx_putdec32(wp->n.lat));
  DBG(tx_puts("\r\n## lon:"));
  DBG(tx_putdec32(wp->n.lon));
  DBG(tx_puts("\r\n"));

  DBG(tx_puts(scan_buffer));
  DBG(tx_puts("\r\n"));

  csv_accum = 0;
  c = csv_numeric(p, 5);
  if (c) {
    wp->sample_rate = csv_accum;
    p += c;
  }
  if (*p != ',') return(current_directive = REPL_ERR_SYNTAX);
  p++;                                /* eat the comma */

  csv_accum = 0;
  c = csv_numeric(p, 5);
  if (c) {
    wp->depth_cm = csv_accum;
    p += c;
  }
  DBG(tx_msg("##depth_cm=", wp->depth_cm));

  /* $end does not have a date or time stamp */
  if (!flag_end) {
    if (*p != ',') return(current_directive = REPL_ERR_SYNTAX);
    p++;                              /* eat the comma */


    DBG(tx_puts(p));
    have_date = csv_dateortime(p, dt);
    DBG(tx_msg("##have_date=", have_date));
    p += have_date;
    if (*p != ',') return(current_directive = REPL_ERR_SYNTAX);
    p++;
    DBG(tx_puts(p));
    have_time = csv_dateortime(p, tm);
    DBG(tx_msg("##have_time=", have_time));
    p += have_time;

    if (have_date && have_time) {
      wp->until = cal_seconds(dt, tm);
      if (!wp->until) {
        syslog_attr("Err:Repl_Time_convert_at", line_nr);
        return(current_directive = REPL_ERR_CONVERSION);
      }
    } else if ((!have_date) && (!have_time)) {
      wp->until = 0;
    } else {
      syslog_attr("Err:Repl_Time_missing_at", line_nr);
      return(current_directive = REPL_ERR_DATETIMESPEC);
    }
  }

  if (*p > ' ') {
    syslog_attr("trailing_junk_char", *p);
    syslog_attr("Err:Repl_trailingJunk_at", line_nr);
    return(current_directive = REPL_ERR_JUNK);
  }

  nr_waypoints++;
  current_directive = (flag_end?REPL_END:REPL_WAYPOINT);
  return(current_directive);
}

int8_t repl_done(struct waypoint * wp, char repl_status)
{
  int8_t er;
  if (repl_status != REPL_DONE_OK &&
      repl_status != REPL_DONE_FAILED)
      return(REPL_ERR_BADSTATUS);
  /* Only mark waypoints, nothing else */
  if (current_directive != REPL_WAYPOINT) return(0);
  DBG(tx_msg("## seeking to ", wp->status_offset));
  er = file_seek(&script, wp->status_offset);
  if (er) goto disk_error;
  sd_buffer[OFFSET(wp->status_offset)] = repl_status;
  er = sd_buffer_sync();
  if (er) goto disk_error;
  return(0);
disk_error:
  syslog_attr("Err:Repl_Done", er);
  syslog_attr("repl_line_nr", line_nr);
  return(er);
}
