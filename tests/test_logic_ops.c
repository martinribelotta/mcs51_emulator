#include "test_common.h"

void test_logic_ops(void)
{
	cpu_t cpu;
	uint8_t code[] = {
		0x54, 0x0F,
		0x42, 0x30,
		0x43, 0x31, 0xF0,
		0x44, 0x01,
		0x62, 0x32,
		0x63, 0x33, 0x0F,
		0x82, 0x00,
		0xA0, 0x01
	};
	load_code(&cpu, code, sizeof(code));
	cpu.acc = 0xF3;
	cpu.iram[0x30] = 0x0F;
	cpu.iram[0x31] = 0x0F;
	cpu.iram[0x32] = 0xF0;
	cpu.iram[0x33] = 0xF0;
	cpu.iram[0x20] = 0x01;

	cpu_step(&cpu);
	ASSERT_EQ("ANL A,#imm", cpu.acc, 0x03);
	cpu.acc = 0xF0;
	cpu_step(&cpu);
	ASSERT_EQ("ORL direct,A", cpu.iram[0x30], 0xFF);
	cpu_step(&cpu);
	ASSERT_EQ("ORL direct,#imm", cpu.iram[0x31], 0xFF);
	cpu.acc = 0x00;
	cpu_step(&cpu);
	ASSERT_EQ("ORL A,#imm", cpu.acc, 0x01);
	cpu.acc = 0xFF;
	cpu_step(&cpu);
	ASSERT_EQ("XRL direct,A", cpu.iram[0x32], 0x0F);
	cpu_step(&cpu);
	ASSERT_EQ("XRL direct,#imm", cpu.iram[0x33], 0xFF);
	cpu_set_carry(&cpu, true);
	cpu_step(&cpu);
	ASSERT_TRUE("ANL C,bit", cpu_get_carry(&cpu));
	cpu_step(&cpu);
	ASSERT_TRUE("ORL C,/bit", cpu_get_carry(&cpu));
}
