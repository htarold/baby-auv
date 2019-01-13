#ifndef PITCH_H
#define PITCH_H

/*
  (C) 2019 Harold Tay LGPLv3
  This module still used by some test binaries, otherwise pitch
  functionalist has been subsumed by ctrl.{c,h}.
 */

extern uint32_t pitch_recently;       /* most recent pitch action */
extern int8_t pitch_level(void);
extern int8_t pitch_up(void);
extern int8_t pitch_down(void);
extern void pitch_init(void);
/* positive means nose DOWN (same sense as mma()) */
#define PITCH_NOSE_UP -100
#define PITCH_NOSE_LEVEL 0
#define PITCH_NOSE_DOWN 100
#define PITCH_TRIM_UP 1
#define PITCH_TRIM_DOWN -1
/* returns -1 if limit reached */
extern int8_t pitch_trim(int8_t inc);

#endif /* PITCH_H */
