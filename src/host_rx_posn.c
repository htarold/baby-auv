/* (C) 2019 Harold Tay LGPLv3 */
/*
  Receive packets from Baby AUV.
  Input/output from stdin/stdout.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include "pkt.h"
#include "crc.h"

static double convert_to_decdeg(uint32_t navt)
{
  return (360.0*navt)/(1<<26);
}

static uint16_t in(void)
{
  uint8_t ch;
  int er;
  er = read(0, &ch, 1);
  if (1 == er) {
    fprintf(stderr, "0x%x ", (unsigned)(uint8_t)ch);
    return(ch);
  }
  if (0 == er)
    fprintf(stderr, "Input closed\n");
  else
    fprintf(stderr, "Error reading input: %s\n", strerror(errno));
  return(0xffff);
}

char buf[64];

int
main(int argc, char ** argv)
{
  printf("sizeof(struct nav_pt) = %lu\n", sizeof(struct nav_pt));
  printf("sizeof(struct header) = %lu\n", sizeof(struct header));
  printf("sizeof(struct position) = %lu\n", sizeof(struct position));
  for ( ; ; ) {
    int er;
    uint16_t id;
    er = pkt_read(buf, &id, in);
    printf("er = %d, id = %d\n", er, id);
    if (er == PKT_TYPE_POSN) {
      struct nav_pt * np;
      np = (void *)buf;
      printf("lat = %d, lon = %d\n", np->lat, np->lon);
    }
  }
  exit(0);
}
