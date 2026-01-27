#pragma once

#include <forth_defs.h>
#include <stdint.h>
#include <stdbool.h>

enum {
	FORTH_STA_IDLE = (1 << 0),
	FORTH_STA_DUMP = (1 << 1),
	FORTH_STA_PSER = (1 << 2),
	FORTH_STA_RSER = (1 << 3),
};

struct forth_context {
	uint32_t psp;
	uint32_t ps0;
	uint32_t rsp;
	uint32_t rs0;
	uint32_t tos;
	uint32_t w;
	uint32_t ip;
	uint32_t sta;
};

extern uint32_t FORTH_SELFTEST[];

extern void forth_init(void);
extern void forth_selftest(void);
