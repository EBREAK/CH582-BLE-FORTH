#pragma once

#include "CH58x_common.h"
#include <stdbool.h>
#include <stdint.h>

extern uint8_t __kpram_start[];
extern uint8_t __kpram_end[];

#define KPRAM_MAGIC 0xDEADBEEF

struct __PACKED kpram_context {
	volatile uint32_t magic;
};

extern struct __PACKED kpram_context *kpram;

extern void kpram_init(void);
