#include <stdio.h>
#include "CH58x_common.h"

void debug_putc(uint8_t c)
{
	while (R8_UART1_TFC >= UART_FIFO_SIZE)
		;
	R8_UART1_THR = c;
}
void debug_puts(char *s)
{
	while (s[0] != 0) {
		debug_putc(s[0]);
		s += 1;
	}
}
char num2hex_lut[] = "0123456789ABCDEF";
void debug_puthex(uint32_t n)
{
	int shi = 28;
	while (shi >= 0) {
		debug_putc(num2hex_lut[(n >> shi) & 0xF]);
		shi -= 4;
	}
}
