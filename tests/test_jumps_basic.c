#include "test_common.h"

void test_jumps_basic(void)
{
	cpu_t cpu;

	uint8_t code_jc[] = { 0x40, 0x02 };
	load_code(&cpu, code_jc, sizeof(code_jc));
	cpu_set_carry(&cpu, true);
	cpu_step(&cpu);
	ASSERT_EQ("JC taken", cpu.pc, 4);

	uint8_t code_jnc[] = { 0x50, 0x02 };
	load_code(&cpu, code_jnc, sizeof(code_jnc));
	cpu_set_carry(&cpu, false);
	cpu_step(&cpu);
	ASSERT_EQ("JNC taken", cpu.pc, 4);

	uint8_t code_jz[] = { 0x60, 0x02 };
	load_code(&cpu, code_jz, sizeof(code_jz));
	cpu.acc = 0;
	cpu_step(&cpu);
	ASSERT_EQ("JZ taken", cpu.pc, 4);

	uint8_t code_jnz[] = { 0x70, 0x02 };
	load_code(&cpu, code_jnz, sizeof(code_jnz));
	cpu.acc = 1;
	cpu_step(&cpu);
	ASSERT_EQ("JNZ taken", cpu.pc, 4);
}
