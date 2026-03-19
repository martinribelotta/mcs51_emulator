#include "test_common.h"

void test_da_flags(void)
{
	cpu_t cpu;
	uint8_t code[] = { 0xD4 };
	load_code(&cpu, code, sizeof(code));
	cpu.acc = 0x9B;
	cpu.psw |= 0x40;
	cpu_step(&cpu);
	ASSERT_EQ("DA carry", cpu.psw & 0x80, 0x80);
}
