#include "test_common.h"

void test_add_addc_subb(void)
{
	cpu_t cpu;
	uint8_t code[] = { 0x24, 0x02, 0x34, 0x00, 0x94, 0x01 };
	load_code(&cpu, code, sizeof(code));
	cpu.acc = 0xFE;
	cpu_step(&cpu);
	ASSERT_EQ("ADD A,#imm", cpu.acc, 0x00);
	ASSERT_TRUE("ADD carry", cpu_get_carry(&cpu));
	cpu_set_carry(&cpu, true);
	cpu_step(&cpu);
	ASSERT_EQ("ADDC A,#imm", cpu.acc, 0x01);
	cpu_set_carry(&cpu, false);
	cpu_step(&cpu);
	ASSERT_EQ("SUBB A,#imm", cpu.acc, 0x00);
}
