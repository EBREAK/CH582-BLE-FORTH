#include "CH58x_common.h"
#include "debug.h"

__INTERRUPT
__HIGH_CODE
void HardFault_Handler(void)
{
	debug_puts("\r\nHARDFAULT\r\n");
	debug_puts("MCAUSE: ");
	debug_puthex(__get_MCAUSE());
	debug_puts("\r\nMEPC: ");
	debug_puthex(__get_MEPC());
	debug_puts("\r\nMTVAL: ");
	debug_puthex(__get_MTVAL());
	debug_puts("\r\n");
#if (DEBUG == Debug_UART1)
	while ((R8_UART1_LSR & RB_LSR_TX_ALL_EMP) == 0) {
		__nop();
	}
#endif
	DelayMs(1000);
	FLASH_ROM_SW_RESET();
	sys_safe_access_enable();
	R16_INT32K_TUNE = 0xFFFF;
	sys_safe_access_disable();
	sys_safe_access_enable();
	R8_RST_WDOG_CTRL |= RB_SOFTWARE_RESET;
	sys_safe_access_disable();
	while (1)
		;
}
