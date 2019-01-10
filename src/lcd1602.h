/* (C) 2019 Harold Tay LGPLv3 */
#ifndef LCD1602_H
#define LCD1602_H

/*
  Change to match your board.  This interface only writes, never
  reads.
  User must call i2c_init(), this is not done here.
 */
#define LCD1602_WRADDR 0x4e

/*
  All functions return 0 on success.
 */
/*
  Initialises th LCD module.
 */
extern int8_t lcd_init(void);

/*
  Turn on (1) or off (0).
 */
extern void lcd_backlight(uint8_t on);

/*
  Outputs a single char at the current position.
 */
extern int8_t lcd_put(uint8_t ch);

/*
  Clears to end of current line, starts a
  new line (could be same line, 0 or 1).
 */
extern int8_t lcd_clrtoeol(void);

/*
  Go to arbitrary line (0 or 1), column (0 to 39).
 */
extern int8_t lcd_goto(uint8_t line_nr, uint8_t column);
#define lcd_newline(line) lcd_goto(line, 0)
#endif /* LCD1602_h */
