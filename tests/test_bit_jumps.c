#include "test_common.h"

void test_bit_jumps(void)
{
	cpu_t cpu;
	uint8_t code_jb[] = { 0x20, 0x00, 0x02 };
	load_code(&cpu, code_jb, sizeof(code_jb));
	cpu.iram[0x20] = 0x01;
	cpu_step(&cpu);
	ASSERT_EQ("JB taken", cpu.pc, 5);

	uint8_t code_jbc[] = { 0x10, 0x00, 0x02 };
	load_code(&cpu, code_jbc, sizeof(code_jbc));
	cpu.iram[0x20] = 0x01;
	cpu_step(&cpu);
	ASSERT_EQ("JBC taken", cpu.pc, 5);
	ASSERT_EQ("JBC cleared", cpu.iram[0x20] & 0x01, 0x00);

	uint8_t code_jnb[] = { 0x30, 0x00, 0x02 };
	load_code(&cpu, code_jnb, sizeof(code_jnb));
	cpu.iram[0x20] = 0x00;
	cpu_step(&cpu);
	ASSERT_EQ("JNB taken", cpu.pc, 5);
}
