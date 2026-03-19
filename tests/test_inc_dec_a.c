#include "test_common.h"

void test_inc_dec_a(void)
{
	cpu_t cpu;
	uint8_t code[] = { 0x04, 0x14 };
	load_code(&cpu, code, sizeof(code));
	cpu.acc = 0x0F;
	cpu_step(&cpu);
	ASSERT_EQ("INC A", cpu.acc, 0x10);
	cpu_step(&cpu);
	ASSERT_EQ("DEC A", cpu.acc, 0x0F);
}
