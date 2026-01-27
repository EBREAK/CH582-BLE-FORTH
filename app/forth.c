#include "forth.h"
#include "debug.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

static inline void PPUSH(struct forth_context *fctx, uint32_t value)
{
	if (fctx->psp < fctx->ps0) {
		fctx->sta |= FORTH_STA_PSER;
		return;
	}
	*(uint32_t *)fctx->psp = value;
	fctx->psp += 4;
}

static inline uint32_t PPOP(struct forth_context *fctx)
{
	fctx->psp -= 4;
	if (fctx->psp < fctx->ps0) {
		fctx->sta |= FORTH_STA_PSER;
		return 0xBAADBEEF;
	}
	return *(uint32_t *)fctx->psp;
}

static inline void RPUSH(struct forth_context *fctx, uint32_t value)
{
	if (fctx->rsp < fctx->rs0) {
		fctx->sta |= FORTH_STA_RSER;
		return;
	}
	*(uint32_t *)fctx->rsp = value;
	fctx->rsp += 4;
}

static inline uint32_t RPOP(struct forth_context *fctx)
{
	fctx->rsp -= 4;
	if (fctx->rsp < fctx->rs0) {
		fctx->sta |= FORTH_STA_RSER;
		return 0xBAADBEEF;
	}
	return *(uint32_t *)fctx->rsp;
}

void forth_dump(struct forth_context *fctx)
{
	debug_puts("FORTH:");
	debug_puts(" CTX ");
	debug_puthex((uint32_t)fctx);
	debug_puts(" IP ");
	debug_puthex(fctx->ip);
	debug_puts(" PSP ");
	debug_puthex(fctx->psp);
	debug_puts(" RSP ");
	debug_puthex(fctx->rsp);
	debug_puts(" STA ");
	debug_puthex(fctx->sta);
	debug_puts("\r\n");
}

void C_NOOP(struct forth_context *fctx)
{
}

void C_BRANCH(struct forth_context *fctx)
{
	fctx->ip = *(uint32_t *)fctx->ip;
}

void C_PAUSE(struct forth_context *fctx)
{
	fctx->sta |= FORTH_STA_IDLE;
}

void C_EQCHK(struct forth_context *fctx)
{
	uint32_t a, b;
	a = PPOP(fctx);
	b = fctx->tos;
	fctx->tos = PPOP(fctx);
	if (a != b) {
		debug_puts("FORTH: EQCHK FAIL\r\n");
		forth_dump(fctx);
	}
	assert(a == b);
}

void C_PZCHK(struct forth_context *fctx)
{
	bool flag = true;
	flag &= (fctx->sta & FORTH_STA_PSER) == 0;
	flag &= fctx->tos == FORTH_TOS_INIT;
	flag &= fctx->psp == fctx->ps0;
	if (flag == false) {
		debug_puts("FORTH: PZCHK FAIL\r\n");
		forth_dump(fctx);
	}
	assert((fctx->sta & FORTH_STA_PSER) == 0);
	assert(fctx->tos == FORTH_TOS_INIT);
	assert(fctx->psp == fctx->ps0);
}

void C_LIT(struct forth_context *fctx)
{
	PPUSH(fctx, fctx->tos);
	fctx->tos = *(uint32_t *)fctx->ip;
	fctx->ip += 4;
}

void C_TRUE(struct forth_context *fctx)
{
	PPUSH(fctx, fctx->tos);
	fctx->tos = FORTH_TRUE;
}

void C_FALSE(struct forth_context *fctx)
{
	PPUSH(fctx, fctx->tos);
	fctx->tos = FORTH_FALSE;
}

void C_0BRANCH(struct forth_context *fctx)
{
	uint32_t flag;
	flag = fctx->tos;
	fctx->tos = PPOP(fctx);
	if (flag) {
		fctx->ip += 4;
		return;
	}
	fctx->ip = *(uint32_t *)fctx->ip;
}

void C_DOCONST(struct forth_context *fctx)
{
	PPUSH(fctx, fctx->tos);
	fctx->tos = *(uint32_t *)(fctx->w + 4);
}

void C_DROP(struct forth_context *fctx)
{
	fctx->tos = PPOP(fctx);
}

void C_DUP(struct forth_context *fctx)
{
	PPUSH(fctx, fctx->tos);
}

void C_SWAP(struct forth_context *fctx)
{
	register uint32_t a;
	a = PPOP(fctx);
	PPUSH(fctx, fctx->tos);
	fctx->tos = a;
}

void C_NIP(struct forth_context *fctx)
{
	PPOP(fctx);
}

void C_OVER(struct forth_context *fctx)
{
	register uint32_t a;
	a = PPOP(fctx);
	PPUSH(fctx, a);
	PPUSH(fctx, fctx->tos);
	fctx->tos = a;
}

void C_ROT(struct forth_context *fctx)
{
	register uint32_t a, b;
	b = PPOP(fctx);
	a = PPOP(fctx);
	PPUSH(fctx, b);
	PPUSH(fctx, fctx->tos);
	fctx->tos = a;
}

void C_NROT(struct forth_context *fctx)
{
	register uint32_t a, b;
	b = PPOP(fctx);
	a = PPOP(fctx);
	PPUSH(fctx, fctx->tos);
	PPUSH(fctx, a);
	fctx->tos = b;
}

void C_TOR(struct forth_context *fctx)
{
	RPUSH(fctx, fctx->tos);
	fctx->tos = PPOP(fctx);
}

void C_FROMR(struct forth_context *fctx)
{
	PPUSH(fctx, fctx->tos);
	fctx->tos = RPOP(fctx);
}

void C_2DROP(struct forth_context *fctx)
{
	PPOP(fctx);
	fctx->tos = PPOP(fctx);
}

void C_2DUP(struct forth_context *fctx)
{
	register uint32_t a;
	a = PPOP(fctx);
	PPUSH(fctx, a);
	PPUSH(fctx, fctx->tos);
	PPUSH(fctx, a);
}

void C_2SWAP(struct forth_context *fctx)
{
	register uint32_t a, b, c;
	c = PPOP(fctx);
	b = PPOP(fctx);
	a = PPOP(fctx);
	PPUSH(fctx, c);
	PPUSH(fctx, fctx->tos);
	PPUSH(fctx, a);
	fctx->tos = b;
}

void C_2OVER(struct forth_context *fctx)
{
	register uint32_t a, b, c;
	c = PPOP(fctx);
	b = PPOP(fctx);
	a = PPOP(fctx);
	PPUSH(fctx, a);
	PPUSH(fctx, b);
	PPUSH(fctx, c);
	PPUSH(fctx, fctx->tos);
	PPUSH(fctx, a);
	fctx->tos = b;
}

void C_2ROT(struct forth_context *fctx)
{
	register uint32_t a, b, c, d, e;
	e = PPOP(fctx);
	d = PPOP(fctx);
	c = PPOP(fctx);
	b = PPOP(fctx);
	a = PPOP(fctx);
	PPUSH(fctx, c);
	PPUSH(fctx, d);
	PPUSH(fctx, e);
	PPUSH(fctx, fctx->tos);
	PPUSH(fctx, a);
	fctx->tos = b;
}

void C_2NROT(struct forth_context *fctx)
{
	register uint32_t a, b, c, d, e;
	e = PPOP(fctx);
	d = PPOP(fctx);
	c = PPOP(fctx);
	b = PPOP(fctx);
	a = PPOP(fctx);
	PPUSH(fctx, e);
	PPUSH(fctx, fctx->tos);
	PPUSH(fctx, a);
	PPUSH(fctx, b);
	PPUSH(fctx, c);
	fctx->tos = d;
}

void C_PICK(struct forth_context *fctx)
{
	fctx->tos = *(uint32_t *)(fctx->psp - ((fctx->tos + 1) * 4));
}

void C_3DROP(struct forth_context *fctx)
{
	PPOP(fctx);
	PPOP(fctx);
	fctx->tos = PPOP(fctx);
}

void C_PLUS(struct forth_context *fctx)
{
	register uint32_t a;
	a = PPOP(fctx);
	fctx->tos = a + fctx->tos;
}

void C_MINUS(struct forth_context *fctx)
{
	register uint32_t a;
	a = PPOP(fctx);
	fctx->tos = a - fctx->tos;
}

void C_MULTI(struct forth_context *fctx)
{
	register uint32_t a;
	a = PPOP(fctx);
	fctx->tos = a * fctx->tos;
}

void C_DIVID(struct forth_context *fctx)
{
	register uint32_t a;
	a = PPOP(fctx);
	fctx->tos = a / fctx->tos;
}

void C_MOD(struct forth_context *fctx)
{
	register uint32_t a;
	a = PPOP(fctx);
	fctx->tos = a % fctx->tos;
}

void C_LSHIFT(struct forth_context *fctx)
{
	register uint32_t a;
	a = PPOP(fctx);
	fctx->tos = a << fctx->tos;
}

void C_RSHIFT(struct forth_context *fctx)
{
	register uint32_t a;
	a = PPOP(fctx);
	fctx->tos = a >> fctx->tos;
}

void C_AND(struct forth_context *fctx)
{
	register uint32_t a;
	a = PPOP(fctx);
	fctx->tos = a & fctx->tos;
}

void C_OR(struct forth_context *fctx)
{
	register uint32_t a;
	a = PPOP(fctx);
	fctx->tos = a | fctx->tos;
}

void C_XOR(struct forth_context *fctx)
{
	register uint32_t a;
	a = PPOP(fctx);
	fctx->tos = a ^ fctx->tos;
}

void C_INVERT(struct forth_context *fctx)
{
	fctx->tos = 0xFFFFFFFF ^ fctx->tos;
}

void C_NEGATE(struct forth_context *fctx)
{
	fctx->tos = -(fctx->tos);
}

void C_ABS(struct forth_context *fctx)
{
	fctx->tos = abs((int32_t)fctx->tos);
}

void C_1PLUS(struct forth_context *fctx)
{
	fctx->tos += 1;
}

void C_2PLUS(struct forth_context *fctx)
{
	fctx->tos += 2;
}

void C_4PLUS(struct forth_context *fctx)
{
	fctx->tos += 4;
}

void C_1MINUS(struct forth_context *fctx)
{
	fctx->tos -= 1;
}

void C_2MINUS(struct forth_context *fctx)
{
	fctx->tos -= 2;
}

void C_4MINUS(struct forth_context *fctx)
{
	fctx->tos -= 4;
}

void C_2MULTI(struct forth_context *fctx)
{
	fctx->tos *= 2;
}

void C_4MULTI(struct forth_context *fctx)
{
	fctx->tos *= 4;
}

void C_2DIVID(struct forth_context *fctx)
{
	fctx->tos /= 2;
}

void C_4DIVID(struct forth_context *fctx)
{
	fctx->tos /= 4;
}

void C_EQ(struct forth_context *fctx)
{
	register uint32_t a, b;
	b = fctx->tos;
	a = PPOP(fctx);
	fctx->tos = FORTH_FALSE;
	if (a == b) {
		fctx->tos = FORTH_TRUE;
	}
}

void C_NE(struct forth_context *fctx)
{
	register uint32_t a, b;
	b = fctx->tos;
	a = PPOP(fctx);
	fctx->tos = FORTH_FALSE;
	if (a != b) {
		fctx->tos = FORTH_TRUE;
	}
}

void C_SLT(struct forth_context *fctx)
{
	register int32_t a, b;
	b = fctx->tos;
	a = PPOP(fctx);
	fctx->tos = FORTH_FALSE;
	if (a < b) {
		fctx->tos = FORTH_TRUE;
	}
}

void C_ULT(struct forth_context *fctx)
{
	register uint32_t a, b;
	b = fctx->tos;
	a = PPOP(fctx);
	fctx->tos = FORTH_FALSE;
	if (a < b) {
		fctx->tos = FORTH_TRUE;
	}
}

void C_SGT(struct forth_context *fctx)
{
	register int32_t a, b;
	b = fctx->tos;
	a = PPOP(fctx);
	fctx->tos = FORTH_FALSE;
	if (a > b) {
		fctx->tos = FORTH_TRUE;
	}
}

void C_UGT(struct forth_context *fctx)
{
	register uint32_t a, b;
	b = fctx->tos;
	a = PPOP(fctx);
	fctx->tos = FORTH_FALSE;
	if (a > b) {
		fctx->tos = FORTH_TRUE;
	}
}

void C_SMIN(struct forth_context *fctx)
{
	register int32_t a;
	a = PPOP(fctx);
	if (a < (int32_t)fctx->tos) {
		fctx->tos = a;
	}
}


void C_UMIN(struct forth_context *fctx)
{
	register uint32_t a;
	a = PPOP(fctx);
	if (a < fctx->tos) {
		fctx->tos = a;
	}
}

void C_SMAX(struct forth_context *fctx)
{
	register int32_t a;
	a = PPOP(fctx);
	if (a > (int32_t)fctx->tos) {
		fctx->tos = a;
	}
}

void C_UMAX(struct forth_context *fctx)
{
	register uint32_t a;
	a = PPOP(fctx);
	if (a > fctx->tos) {
		fctx->tos = a;
	}
}

void C_EQZ(struct forth_context *fctx)
{
	register uint32_t a;
	a = fctx->tos;
	fctx->tos = FORTH_FALSE;
	if (a == 0) {
		fctx->tos = FORTH_TRUE;
	}
}

void C_NEZ(struct forth_context *fctx)
{
	register uint32_t a;
	a = fctx->tos;
	fctx->tos = FORTH_FALSE;
	if (a != 0) {
		fctx->tos = FORTH_TRUE;
	}
}

void C_LTZ(struct forth_context *fctx)
{
	register int32_t a;
	a = fctx->tos;
	fctx->tos = FORTH_FALSE;
	if (a < 0) {
		fctx->tos = FORTH_TRUE;
	}
}


void C_GTZ(struct forth_context *fctx)
{
	register int32_t a;
	a = fctx->tos;
	fctx->tos = FORTH_FALSE;
	if (a > 0) {
		fctx->tos = FORTH_TRUE;
	}
}

void C_UCLOAD(struct forth_context *fctx)
{
	fctx->tos = *(uint8_t *)fctx->tos;
}

void C_SCLOAD(struct forth_context *fctx)
{
	fctx->tos = *(int8_t *)fctx->tos;
}

void C_UWLOAD(struct forth_context *fctx)
{
	fctx->tos = *(uint16_t *)fctx->tos;
}

void C_SWLOAD(struct forth_context *fctx)
{
	fctx->tos = *(int16_t *)fctx->tos;
}

void C_ULLOAD(struct forth_context *fctx)
{
	fctx->tos = *(uint32_t *)fctx->tos;
}

void C_SLLOAD(struct forth_context *fctx)
{
	fctx->tos = *(int32_t *)fctx->tos;
}

void forth_run(struct forth_context *fctx)
{
	void (*entr)(struct forth_context *fctx);
	while ((fctx->sta & FORTH_STA_IDLE) == 0) {
		if (fctx->sta & FORTH_STA_DUMP) {
			forth_dump(fctx);
		}
		fctx->w = *(uint32_t *)fctx->ip;
		fctx->ip += 4;
		entr = (void *)(*(uint32_t *)fctx->w & 0x2007FFFF);
		entr(fctx);
	}
}

void forth_init(void)
{
}

void forth_selftest(void)
{
	uint32_t forth_test_pstk[FORTH_STACK_DEPTH + 1];
	uint32_t forth_test_rstk[FORTH_STACK_DEPTH + 1];
	struct forth_context forth_test;
	bzero(&forth_test, sizeof(forth_test));
	forth_test.ps0 = (uint32_t)&forth_test_pstk[1];
	forth_test.psp = forth_test.ps0;
	forth_test.rs0 = (uint32_t)&forth_test_rstk[1];
	forth_test.rsp = forth_test.rs0;
	forth_test.tos = FORTH_TOS_INIT;
	forth_test.ip = (uint32_t)&FORTH_SELFTEST[0];
	//forth_test.sta |= FORTH_STA_DUMP;
	struct forth_context *fctx;
	fctx = &forth_test;
	PPUSH(fctx, 0x55AAFF00);
	assert(fctx->psp != fctx->ps0);
	assert(fctx->psp == (fctx->ps0 + 4));
	assert(PPOP(fctx) == 0x55AAFF00);
	assert(fctx->psp == fctx->ps0);
	RPUSH(fctx, 0xBAADF00D);
	assert(fctx->rsp != fctx->rs0);
	assert(fctx->rsp == (fctx->rs0 + 4));
	assert(RPOP(fctx) == 0xBAADF00D);
	assert(fctx->rsp == fctx->rs0);
	forth_run(fctx);
}
