/* (C) 2019 Harold Tay LGPLv3 */
#ifndef CTRL_H
#define CTRL_H

#define CTRL_ERR_POSE    -1           /* AUV pose beyond controllable */
#define CTRL_ERR_TUDE    -2           /* error reading attitude */
#define CTRL_ERR_CTD     -3           /* error reading CTD */
#define CTRL_ERR_PITCH   -4           /* pitch or tude error */
#define CTRL_ERR_TIMEOUT -5           /* timed out twirling */
#define CTRL_ERR_AIM     -6           /* twirl action not successful */

#define CTRL_CMD_TWIRL    1
#define CTRL_CMD_PITCH    2
#define CTRL_CMD_THRUST   4           /* used with CTRL_CMD_PITCH */

/*
  cmd is bitwise OR of CTRL_CMD_TWIRL, _PITCH, _THRUST
  hdg must be valid if _TWIRL is given. _THRUST makes sense only
  if _PITCH is also given.
 */
extern int8_t ctrl_combo(uint8_t cmd, int8_t hdg);
extern void ctrl_thruster(int8_t percent, int8_t walk);

extern void ctrl_init(void);

/*
  Can take a while to return.  Even if successful, auv may still be
  badly aimed if calibration is off.  Caller can abort and call
  again.
 */
extern int8_t ctrl_twirl(uint8_t heading);

/*
  Call repeatedly to maintain heading and depth.  Will take a
  few seconds to return, depending on sensors' polling schedule.
 */
extern int8_t ctrl_steady(uint8_t hdg_sp, int16_t cm_sp);

#endif /* CTRL_H */
