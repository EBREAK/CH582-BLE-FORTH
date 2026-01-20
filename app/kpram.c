#include "CH58x_common.h"
#include "kpram.h"
#include "debug.h"
#include "crc.h"

#include <string.h>

struct __PACKED kpram_context *kpram = (void *)__kpram_start;
uint32_t kpram_size = 0;

uint8_t kpram_crc_gen(void)
{
	uint8_t crc = 0x00;
	uint32_t i;
	for (i = 0; i < kpram_size; i++) {
		crc = crc8_ccitt_byte(crc, __kpram_start[i]);
	}
	return crc;
}

uint8_t kpram_crc_get(void)
{
	return R8_GLOB_RESET_KEEP;
}

void kpram_crc_set(uint8_t crc)
{
	R8_GLOB_RESET_KEEP = crc;
}

bool kpram_crc_chk(void)
{
	return (kpram_crc_gen() == kpram_crc_get());
}

void kpram_crc_upd(void)
{
	kpram_crc_set(kpram_crc_gen());
}

void kpram_rst(void)
{
	memset(kpram, 0, kpram_size);
	kpram->magic = KPRAM_MAGIC;
	kpram_crc_upd();
	debug_puts("KPRAM RESET\r\n");
}

void kpram_init(void)
{
	kpram_size = (uint32_t)(__kpram_end - __kpram_start);
	if ((R8_RESET_STATUS & RB_RESET_FLAG) == 0b001) {
		debug_puts("KPRAM FIRST BOOT\r\n");
		kpram_rst();
	}
	if (kpram->magic != KPRAM_MAGIC) {
		debug_puts("KPRAM INVALID MAGIC\r\n");
		kpram_rst();
	}
	if (kpram_crc_chk() == false) {
		debug_puts("KPRAM INVALID CRC\r\n");
		kpram_rst();
	}

}
