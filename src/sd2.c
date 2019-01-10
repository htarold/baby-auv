/* (C) 2019 Harold Tay LGPLv3 */
#include <avr/io.h>
#include <util/delay.h>
#include "handydefs.h"
#include "sd2.h"
#include "time.h"                     /* for timing of timeouts */
#include "tx.h"

#undef DBG_SD
#ifdef DBG_SD

#define dbg_msg(s, v) do { tx_msg(s, v); } while(0)
#define dbg(a) a
#define put(x) \
do { tx_putc('o'); tx_puthex(x); xfer(x); } while (0)
#define get() \
({uint8_t x = xfer(0xff); tx_putc('i'); tx_puthex(x); x; })

#else

#define dbg_msg(s, v) ; /* nothing */
#define dbg(a) ; /* nothing */
#define put(x) (void)xfer(x)
#define get() xfer(0xff)

#endif

  /*
    SPI2X SPR1 SPR0 SPI SCK frequency
    0 0 0 /4   = 2MHz  (F_CPU = 8MHz)
    0 0 1 /16  = 500kHz
    0 1 0 /64  = 125kHz
    0 1 1 /128 = 62.5kHz
    1 0 0 /2   = 4MHz
    1 0 1 /8   = 1MHz
    1 1 0 /32  = 250kHz
    1 1 1 /64  = 125kHz
    /32 sometimes ok; /8 too fast; /16 too fast.
    Apparently it's not a speed issue; cannot reliably replicate
    problem.  Could be a thermal issue?  It often starts timing
    out after 3000 lines (of a 4000 line test).
   */

#define SPEEDBITS(op0, op1, op2) \
SPSR op0 (1<<SPI2X); SPCR op1 (1<<SPR1); SPCR op2 (1<<SPR1);
#define SPIDIVBY128 SPEEDBITS(&=~,  |=,  |=)
#define SPIDIVBY64  SPEEDBITS(&=~,  |=,  &=~)
#define SPIDIVBY32  SPEEDBITS(|=,   |=,  &=~)
#define SPIDIVBY16  SPEEDBITS(&=~,  &=~, |=)
#define SPIDIVBY8   SPEEDBITS(|=,   &=~, |=)
#define SPIDIVBY4   SPEEDBITS(&=~,  &=~, &=~)
#define SPIDIVBY2   SPEEDBITS(|=,   &=~, &=~)

uint8_t sd_buffer[512];

static void spi_init(void)
{
  GPBIT_OUTPUT(SD_MOSI);
  GPBIT_OUTPUT(SD_SS);                /* REQUIRED by mega328 */
  GPBIT_SET(SD_SS);
  GPBIT_INPUT(SD_MISO);
  GPBIT_CLR(SD_CS);
  GPBIT_OUTPUT(SD_CS);
  SPCR = _BV(SPE) /* CPOL = CPHA = 0 */ | _BV(MSTR);
  SPIDIVBY2;
  tx_strlit("spi_init returning\r\n");
}
static void cs_lo(void)
{
  GPBIT_CLR(SD_CS);
}
static void cs_hi(void)
{
  GPBIT_SET(SD_CS);
}
static uint8_t xfer(uint8_t x)
{
  SPDR = x;
  while (!(SPSR & _BV(SPIF)));
  return(SPDR);
}

/*
  For timeout purposes, assuming /2 and 8MHz, 1 byte takes 2us to
  clock out, and 1ms is 500 bytes.  Clock in multiples of 10
  bytes.  If we clock slower than this, timeouts will only be
  longer.
 */
#define SKIP_MS(skip, ms) skip_(skip, (uint16_t)(ms*50))
static uint8_t skip_(uint8_t skip, uint16_t x10bytes)
{
  uint8_t i, res;
  do {
    for(i = 0; i < 10; i++) {
      res = xfer(0xff);
      if (res != skip) goto done;
    }
  } while (x10bytes--);
done:
  return(res);
}

static void putdw(uint32_t dword)
{
  put((dword>>24) & 0xff);
  put((dword>>16) & 0xff);
  put((dword>>8) & 0xff);
  put(dword & 0xff);
}

uint8_t sd_result;
uint32_t sd_response;
static uint8_t cmd(uint8_t c, uint32_t arg, uint8_t crc)
{
  uint8_t i;

  put(c|(1<<6));
  putdw(arg);
  put(crc);

  sd_result = SKIP_MS(0xff, 50);

  sd_response = get();  sd_response <<= 8;
  sd_response |= get(); sd_response <<= 8;
  sd_response |= get(); sd_response <<= 8;
  sd_response |= get();
  for(i = 0; i < 200; i++)
    if (0xff == xfer(0xff)) break;
  return(sd_result);
}

static void delay(uint8_t i)
{
  for ( ; i > 0; i--)
    _delay_ms(8);
}
static int8_t init(void)
{
  uint8_t i;

  (void)SKIP_MS(0xff, 0);

  for(i = 0; ; i++, delay(i)) {
    if (10 == i) return(SD_TMO_IDLESTATE0);
    if (1 == cmd(0, 0, 0x95)) break;
  }

  (void)SKIP_MS(0xff, 20);

  for(i = 0; ; i++, delay(i)) {
    if (10 == i) return(SD_TMO_IFCOND8);
    if (1 == cmd(8, 0x1aa, 0x87)) break;
  }
  if (sd_response != 0x1aa) return(SD_ECHO_ERR8);

  for(i = 0; ; i++, delay(2*i)) {
    if (40 == i) return(SD_TMO_APPCMD55);
    if (1 == cmd(55, 0, 0x65))
      if (0 == cmd(41, 0x40000000, 0x77)) break;
  }

  for(i = 0; ; i++, delay(i)) {
    if (10 == i) return(SD_TMO_OCR58);
    if (0 == cmd(58, 0, 0)) break;
  }

  if (!(sd_response & (1UL<<22))) return(SD_NOT_SDHC);

  SPIDIVBY2;
  return(0);
}

int8_t sd_init(void)
{
  int8_t i, res;

  cs_hi();
  for (i = 0; i < 10; i++, delay(i)) {
    dbg_msg("sd_init attempt ", i);
    spi_init();
    cs_lo();
    res = init();
    cs_hi();
    if (!res) break;
    dbg_msg("init error: ", res);
  }
  return(res);
}

static int8_t bread(uint32_t addr)
{
  int8_t res;
  uint16_t i;

  res = 0;

  cs_lo();
  put(17|(1<<6));
  putdw(addr);
  put(0);

  sd_result = SKIP_MS(0xff, 300);
  if (0 != sd_result) {
    res = SD_TMO_READBLK;
    goto done;
  }

  sd_result = SKIP_MS(0xff, 300);
  if (0xfe != sd_result) {
    res = SD_TMO_READBLK2;
    goto done;
  }

  for(i = 0; i < sizeof(sd_buffer); i++)  
    sd_buffer[i] = xfer(0xff);

  putdw(0xffffffff);                  /* "CRC bytes" */
done:
  cs_hi();
  if (res) { dbg_msg("bread error: ", res); }
  return(res);
}

int8_t sd_bread(uint32_t addr)
{
  int8_t res;
  if ((res = bread(addr)))
    if (!(res = sd_init()))
      res = bread(addr);
  return(res);
}

static int8_t bwrite(uint32_t addr)
{
  int8_t res;
  uint16_t i;

  res = 0;

  cs_lo();
  put(24|(1<<6));
  putdw(addr);

  sd_result = SKIP_MS(0xff, 600);
  if (sd_result != 0) {
    res = SD_TMO_WRITEBLK;
    goto done;
  }

  put(0xfe);                          /* start token */
  for(i = 0; i < sizeof(sd_buffer); i++) {
    (void)xfer(sd_buffer[i]);
  }

  put(0);                             /* "CRC" */
  put(0);

  sd_result = SKIP_MS(0xff, 600);
  if ((sd_result & 0x1f) != 5) {
    res = SD_ERR_WRITEBLK;
    goto done;
  }

  sd_result = SKIP_MS(0, 600);
  if (!sd_result) {
    res = SD_TMO_WRITEBLK2;
    goto done;
  }

done:
  cs_hi();
  if (res) { dbg_msg("bwrite error: ", res); }
  return(res);
}

int8_t sd_bwrite(uint32_t addr)
{
  int8_t res;
  if ((res = bwrite(addr)))
    if (!(res = sd_init()))
      res = bwrite(addr);
  return(res);
}

/*
  sd_buffer[] can support different users.  sd_buffer_checkout(addr)
  fills the buffer from addr after first saving its current
  contents.  sd_buffer_checkin(addr) relinquishes the buffer, whose
  contents are from addr.
 */
uint32_t current_address = SD_ADDRESS_NONE;
int8_t sd_buffer_sync(void)
{
  return (current_address == SD_ADDRESS_NONE?
          0:
          sd_bwrite(current_address));
}
int8_t sd_buffer_checkout(uint32_t addr)
{
  int8_t res;
  if (addr != current_address) {
    res = sd_buffer_sync();
    if (res) return(res);
    if (addr != SD_ADDRESS_NONE) {
      res = sd_bread(addr);
      if (res) return(res);
    }
    current_address = addr;
  }
  return(0);
}

void sd_buffer_checkin(uint32_t addr)
{
  current_address = addr;
}
