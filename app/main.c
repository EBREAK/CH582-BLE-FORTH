#include "HAL.h"
#include "config.h"
#include "debug.h"
#include "kpram.h"
#include "crc.h"
#include "usbdev.h"
#include "fifo.h"
#include "forth.h"

__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

__HIGH_CODE
__attribute__((noinline)) void Main_Circulation()
{
	while (1) {
		TMOS_SystemProcess();
	}
}

int main(void)
{
	PWR_DCDCCfg(ENABLE);
	SetSysClock(CLK_SOURCE_PLL_60MHz);
	GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
	GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
#if (DEBUG == Debug_UART1)
	GPIOA_SetBits(bTXD1);
	GPIOA_ModeCfg(bTXD1, GPIO_ModeOut_PP_5mA);
	UART1_DefInit();
#endif
	CH58X_BLEInit();
	HAL_Init();
	crc_selftest();
	fifo_selftest();
	debug_puts("RESET REASON: ");
	debug_puthex(R8_RESET_STATUS & RB_RESET_FLAG);
	debug_puts("\r\n");
	kpram_init();
	usbdev_init();
	forth_init();
	forth_selftest();
	debug_puts("FORTH ON CH582\r\n");
	Main_Circulation();
}
