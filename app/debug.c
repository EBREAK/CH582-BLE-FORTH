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
