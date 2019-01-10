/* (C) 2019 Harold Tay LGPLv3 */
#include "crc.h"
 /* From "/usr/lib/avr/include/util/crc16.h": */
uint16_t
crc_xmodem_update (uint16_t crc, uint8_t data)
{
    int i;

    crc = crc ^ ((uint16_t)data << 8);
    for (i=0; i<8; i++)
    {
        if (crc & 0x8000)
            crc = (crc << 1) ^ 0x1021;
        else
            crc <<= 1;
    }

    return crc;
}
