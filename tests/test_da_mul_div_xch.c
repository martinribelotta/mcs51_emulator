#include "test_common.h"

void test_da_mul_div_xch(void)
{
	cpu_t cpu;
	uint8_t code[] = { 0xD4, 0xA4, 0x84, 0xC5, 0x30, 0xC6, 0xC8, 0xD6 };
	load_code(&cpu, code, sizeof(code));

	cpu.acc = 0x9B;
	cpu.psw |= 0x40;
	cpu_step(&cpu);
	ASSERT_EQ("DA A", cpu.acc, 0x01);
	ASSERT_TRUE("DA carry", cpu_get_carry(&cpu));

	cpu.acc = 0x10;
	cpu.b = 0x10;
	cpu_step(&cpu);
	ASSERT_EQ("MUL AB acc", cpu.acc, 0x00);
	ASSERT_EQ("MUL AB b", cpu.b, 0x01);

	cpu.acc = 0x10;
	cpu.b = 0x04;
	cpu_step(&cpu);
	ASSERT_EQ("DIV AB acc", cpu.acc, 0x04);
	ASSERT_EQ("DIV AB b", cpu.b, 0x00);

	cpu.acc = 0xAA;
	cpu.iram[0x30] = 0x55;
	cpu_step(&cpu);
	ASSERT_EQ("XCH A,direct acc", cpu.acc, 0x55);
	ASSERT_EQ("XCH A,direct mem", cpu.iram[0x30], 0xAA);

	cpu.iram[0] = 0x40;
	cpu.iram[0x40] = 0x12;
	cpu.acc = 0x34;
	cpu_step(&cpu);
	ASSERT_EQ("XCH A,@Ri", cpu.acc, 0x12);

	cpu.iram[0] = 0x56;
	cpu.acc = 0x78;
	cpu_step(&cpu);
	ASSERT_EQ("XCH A,Rn", cpu.acc, 0x56);

	cpu.iram[0] = 0x41;
	cpu.iram[0x41] = 0xAB;
	cpu.acc = 0xCD;
	cpu_step(&cpu);
	ASSERT_EQ("XCHD A,@Ri", cpu.acc, 0xCB);
	ASSERT_EQ("XCHD A,@Ri mem", cpu.iram[0x41], 0xAD);
}
