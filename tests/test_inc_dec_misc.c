#include "test_common.h"

void test_inc_dec_misc(void)
{
	cpu_t cpu;
	uint8_t code[] = { 0x05, 0x30, 0x15, 0x30, 0xA3, 0xE4 };
	load_code(&cpu, code, sizeof(code));
	cpu.iram[0x30] = 0x01;
	cpu_step(&cpu);
	ASSERT_EQ("INC direct", cpu.iram[0x30], 0x02);
	cpu_step(&cpu);
	ASSERT_EQ("DEC direct", cpu.iram[0x30], 0x01);
	cpu.dptr = 0x1234;
	cpu_step(&cpu);
	ASSERT_EQ("INC DPTR", cpu.dptr, 0x1235);
	cpu.acc = 0xFF;
	cpu_step(&cpu);
	ASSERT_EQ("CLR A", cpu.acc, 0x00);
}
