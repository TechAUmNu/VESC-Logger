#include "crc.h"

unsigned short crc16(const unsigned char* data_p, unsigned int length) {
  unsigned char x;
  unsigned short crc = 0x0000;

  while (length--) {
    x = crc >> 8 ^ *data_p++;
    x ^= x >> 4;
    crc = (crc << 8) ^ ((unsigned short)(x << 12)) ^ ((unsigned short)(x << 5)) ^ ((unsigned short)x);
  }
  return crc;
}
