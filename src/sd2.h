/* (C) 2019 Harold Tay LGPLv3 */
#ifndef SD_H
#define SD_H
#include <stdint.h>

/* SPI pins we use */
#define SD_CS_BIT    PC0
#define SD_CS_PORT   PORTC
#define SD_CS_DDR    DDRC
#define SD_CS_PIN    PINC
#define SD_SS_BIT    PB2
#define SD_SS_PORT   PORTB
#define SD_SS_DDR    DDRB
#define SD_SS_PIN    PINB
#define SD_MOSI_BIT  PB3
#define SD_MOSI_PORT PORTB
#define SD_MOSI_DDR  DDRB
#define SD_MOST_PIN  PINC
#define SD_MISO_BIT  PB4
#define SD_MISO_PORT PORTB
#define SD_MISO_DDR  DDRB
#define SD_MISO_PIN  PINB
#define SD_SCK_BIT   PB5
#define SD_SCK_PORT  PORTB
#define SD_SCK_DDR   DDRB
#define SD_SCK_PIN   PINB

extern uint8_t sd_result;             /* contains last result... */
extern uint32_t sd_response;          /* and last value if any */
#define SD_TMO_IDLESTATE0 -2
#define SD_TMO_IFCOND8    -3
#define SD_ECHO_ERR8      -4
#define SD_TMO_APPCMD55   -5
#define SD_TMO_OCR58      -6
#define SD_NOT_SDHC       -7
#define SD_TMO_READBLK    -8
#define SD_TMO_READBLK2   -9
#define SD_TMO_WRITEBLK   -10
#define SD_TMO_WRITEBLK2  -11
#define SD_ERR_WRITEBLK   -12
extern int8_t sd_init(void);
extern uint8_t sd_buffer[512];        /* all I/O to/from this */
extern int8_t sd_bread(uint32_t addr);
extern int8_t sd_bwrite(uint32_t addr);

#define SD_ADDRESS_NONE (uint32_t)-1
/*
  Check out the data previously checked in under the same
  address.
 */
extern int8_t sd_buffer_checkout(uint32_t addr);
/*
  Give the current buffer a new address.
 */
extern void sd_buffer_checkin(uint32_t addr);

/*
  Sync the current sector to disk.
 */
extern int8_t sd_buffer_sync(void);

#endif /* SD_H */
