/* (C) 2019 Harold Tay LGPLv3 */

/*
  Packet handling routines.
 */

#include <string.h> /* for memcpy() */
#include "pkt.h"
#include "crc.h"

unsigned int
pkt_make_position(uint8_t buffer[], struct nav_pt * np, uint16_t id)
{
  struct position * pp;
  struct header * hp;
  uint16_t crc;
  int8_t i;

  hp = (void *)buffer;
  pp = (void *)(buffer + sizeof (*hp));

  hp->magic = PKT_MAGIC;
  hp->node_id = id;
  hp->type = PKT_TYPE_POSN;
  pp->zero = 0;
  pp->ff = 0xff;
  pp->position = *np;

  crc = 0;
  for (i = 0; i < sizeof(*hp) + sizeof(*pp); i++) {
    crc = crc_xmodem_update(crc, buffer[i]);
  }

  /* XXX Assumes little endian */
  memcpy(buffer + sizeof(*hp) + sizeof(*pp), &crc, 2);
  return(sizeof(*hp) + sizeof(*pp) + 2);
}

/*
  Returns packet type, or error.
 */
int8_t
pkt_read(uint8_t buffer[], uint16_t * idp, uint16_t (*in)(void))
{
  int8_t state, i;
  uint16_t ch, crc, crc_declared, crc_saved;

  crc = crc_declared = crc_saved = 0;

  for (state = i = 0; ; ) {

    ch = in();
    if (0xffff == ch) return(PKT_ER_READ);
    ch &= 0x00ff;
    crc = crc_xmodem_update(crc, (uint8_t)ch);

    switch (state) {
case 0:  /* Get magic */
    if (ch != PKT_MAGIC) return(PKT_ER_MAGIC);
    state++;
    break;
case 1:  /* packet type */
    if (ch != PKT_TYPE_POSN) return(PKT_ER_TYPE);  /* XXX */
    state++;
    break;
case 2:  /* node id */
    *idp = ch;
    state++;
    break;
case 3:
    *idp |= (ch<<8);
    state++;
    break;
case 4:  /* Expect a struct position.  First a zero byte. */
    if (ch) return(PKT_ER_ZERO);
    state++;
    break;
case 5:
    if (ch != 0xff) return(PKT_ER_FF);
    state++;
    break;
case 6:
    i = 0;
    state++;
    /* Fall Through */
case 7:
    buffer[i] = (uint8_t)ch;
    i++;
    crc_saved = crc;
    if (i >= sizeof(struct nav_pt)) state = 100;
    break;
case 100:  /* Check CRC */
    crc_declared = ch;
    state++;
    break;
case 101:
    crc_declared |= (ch<<8);
    if (crc_saved != crc_declared) return(PKT_ER_CRC);
    return(PKT_TYPE_POSN);
    }
  }
}
