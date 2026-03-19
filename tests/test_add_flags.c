#include "test_common.h"

void test_add_flags(void)
{
	cpu_t cpu;
	uint8_t code[] = { 0x24, 0x01, 0x24, 0x01, 0x24, 0x01 };
	load_code(&cpu, code, sizeof(code));

	cpu.acc = 0x0F;
	cpu_step(&cpu);
	ASSERT_EQ("ADD AC", cpu.psw & 0x40, 0x40);
	ASSERT_EQ("ADD CY", cpu.psw & 0x80, 0x00);
	ASSERT_EQ("ADD OV", cpu.psw & 0x04, 0x00);

	cpu.acc = 0x7F;
	cpu_step(&cpu);
	ASSERT_EQ("ADD OV", cpu.psw & 0x04, 0x04);

	cpu.acc = 0xFF;
	cpu_step(&cpu);
	ASSERT_EQ("ADD CY", cpu.psw & 0x80, 0x80);
}
