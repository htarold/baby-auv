#ifndef CBUF_H
#define CBUF_H
/*
  (C) 2019 Harold Tay LGPLv3
  Circular buffer.
  XXX Not exactly threadsafe: write pointer can lap read
  pointer.  Safe if read in time.  With tasks running every
  10ms, 8 bytes is not enough to guarantee no overruns.
 */

/*
  New interface:
 */

#define cbuf_declare(name, size) \
struct name { volatile uint8_t head, tail; volatile char buf[size]; }

#define cbuf_initialise(name) name.head = name.tail = 0;
#define cbuf_put(name, ch) \
  do { name.buf[name.tail++] = ch; \
  name.tail %= sizeof(name.buf); } while (0)
#define cbuf_get(name) \
  ({ char ch = name.buf[name.head++]; \
     name.head %= sizeof(name.buf); ch; })
#define cbuf_haschar(name) (name.head != name.tail)
#define cbuf_nochar(name) (name.head == name.tail)
#define cbuf_isfull(name) \
  (name.head == (name.tail-1)%sizeof(name.buf))

/*
  Deprecated older interface:
 */

struct cbuf { volatile uint8_t buf[16]; volatile uint8_t head, tail; };

#define cbuf_init(cbp) \
do{ (cbp)->head = (cbp)->tail = 0; }while( 0 )

#define cbuf_storechar(cbp, byte) \
do{ (cbp)->buf[(cbp)->tail++] = byte; \
(cbp)->tail %= sizeof((cbp)->buf); }while( 0 )

#define cbuf_notempty(cbp) ((cbp)->head != (cbp)->tail)
#define cbuf_isempty(cbp) ((cbp)->head == (cbp)->tail)

#define cbuf_getchar(cbp) \
({ uint8_t ch = (cbp)->buf[(cbp)->head++]; (cbp)->head %= sizeof((cbp)->buf); ch; })

#endif /* CBUF_H */
