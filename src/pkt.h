/* (C) 2019 Harold Tay LGPLv3 */
#ifndef PKT_H
#define PKT_H
#include <stdint.h>
#include "nav.h"
/*
  All data packet in the air will start with this header.  The
  payload follows, then the CRC (2 bytes).  The length of the
  packet excluding the CRC can be implicit in the type.  Or the
  type may indicate that the length is found in the length field.
  Magic is a constant 85 (= ascii 'U', 0x55) for all packets used
  by Baby AUV.  If packet length is implicit, then the length
  field will be 0.

  CRC is calculated according to util/crc16.h,
  _crc_xmodem_update().

  All multi-byte fields are little endian.
 */
#pragma pack(1)
struct header {
  uint8_t magic;                      /* = 0x55 */
  uint8_t type;
  uint16_t node_id;
};

#define PKT_MAGIC      0x55
#define PKT_TYPE_EDB   0x02
#define PKT_TYPE_POSN  0x03

/*
  If the payload is an encoded data block, the header.type is
  0x02, and header.length is 0.
 */
struct edb {
  uint16_t degree, sequence, current_count;
  uint8_t anon[244];
};

/*
  After edb.anon has been decoded, it is to be interpreted as
  having this structure:
 */

struct decoded {
  struct datum {
    uint32_t reftime;      /* UTC, seconds relative to 2017 */
    uint16_t block_number; /* Starts at 0, increments every block */
    uint16_t auv_id;        /* ... */
    int16_t celsius;       /* degrees/64 */
    uint16_t depth;        /* metres/8 */
    uint16_t conductivity;
    uint16_t paramx;       /* other parameters... */
    uint16_t paramy;
    uint16_t paramz;
    uint16_t revs;
    uint8_t heading;
    int8_t pitch;
  } datum;
  struct delta {
    uint8_t dtime;
    int8_t dcelsius;
    int8_t ddepth;
    int8_t dconductivity;
    int8_t dparamx;
    int8_t dparamy;
    int8_t dparamz;
    int8_t revs;
    uint8_t heading;
    int8_t pitch;
  } delta[22];
};

/*
  To save space, only parameter differences are saved.  These
  are recorded in the delta[] array.

  If delta[i].dtime is 0, then that delta[i] is to be cast as a
  position structure (same size).  In that case, position.ff
  will be all-bits-set (0xff).
 */

struct position {  /* XXX May cause issues with alignment */
  uint8_t zero;
  uint8_t ff;
  struct nav_pt position;
};  /* 10 bytes */

/*
  If delta[i] is unused, then all bits in delta[i] will be 0.

  It can happen that the delta is too big for the 8-bit field.
  Then 2 things can happen:
  1: the rest of delta[] can be 0 and a new struct decoded
  started.
  2: For the d* fields, The current delta[i] carries the low byte,
  and delta[i+1] carries the high byte.  For heading, rpm, and
  pitch, only delta[i+1] carries the valid value.
 */

/*
  If header.type is PKT_TYPE_POSN (0x3), then the payload section
  carries a struct position.
 */

/*
  Packet handling routines.
 */
#define PKT_ER_MAGIC   -10
#define PKT_ER_TYPE    -11
#define PKT_ER_ZERO    -12
#define PKT_ER_FF      -13
#define PKT_ER_CRC     -14
#define PKT_ER_READ    -15

extern unsigned int
pkt_make_position(uint8_t buffer[], struct nav_pt * np, uint16_t id);

extern int8_t
pkt_read(uint8_t buffer[], uint16_t * idp, uint16_t (*in)(void));

#pragma pack()
#endif /* PKT_H */
