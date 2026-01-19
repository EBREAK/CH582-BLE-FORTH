#pragma once

#include <stdint.h>

extern void debug_putc(uint8_t c);
extern void debug_puts(char *s);
extern void debug_puthex(uint32_t n);
