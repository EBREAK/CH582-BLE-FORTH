#pragma once

#include <stdint.h>

extern const uint8_t crc8_ccitt_lut[256];
extern const uint16_t crc16_ccitt_lut[256];

static inline uint8_t crc8_ccitt_byte(uint8_t crc, uint8_t c)
{
	return crc8_ccitt_lut[crc ^ c];
}

static inline uint16_t crc16_ccitt_byte(uint16_t crc, uint8_t c)
{
	return (crc >> 8) ^ crc16_ccitt_lut[(crc ^ c) & 0xff];
}

extern void crc_selftest(void);
