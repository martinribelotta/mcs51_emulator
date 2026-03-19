#include "test_common.h"

void test_nop(void)
{
	cpu_t cpu;
	uint8_t code[] = { 0x00 };
	load_code(&cpu, code, sizeof(code));
	cpu_step(&cpu);
	ASSERT_EQ("NOP pc", cpu.pc, 1);
}
