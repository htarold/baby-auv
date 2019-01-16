/*
  (C) 2019 Harold Tay LGPLv3
  Send a position packet from desktop host using given parameters.
  Primarily for testing purposes.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include "nav.h"
#include "pkt.h"

char * progname;

void usage(void)
{
  fprintf(stderr, "Usage: %s id lat lon [>tty]\n", progname);
  exit(1);
}

int
main(int argc, char ** argv)
{
  int id, len, er;
  struct nav_pt p;
  uint8_t buffer[128];

  progname = argv[0];
  if (argc != 4) usage();

  id = atoi(argv[1]);
  p.lat = atoi(argv[2]);
  p.lon = atoi(argv[3]);

  len = pkt_make_position(buffer, &p, id);
  er = write(1, buffer, len);
  if (er == len) exit(0);
  exit(er);
}
