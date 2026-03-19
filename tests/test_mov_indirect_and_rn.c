#include "test_common.h"

void test_mov_indirect_and_rn(void)
{
	cpu_t cpu;
	uint8_t code[] = {
		0x76, 0x56,
		0xE6,
		0xF6,
		0x86, 0x33,
		0xA6, 0x34,
		0x78, 0x78,
		0xE8,
		0xF8,
		0x88, 0x32,
		0xA8, 0x32
	};
	load_code(&cpu, code, sizeof(code));

	cpu.iram[0] = 0x40;
	cpu_step(&cpu);
	ASSERT_EQ("MOV @Ri,#imm", cpu.iram[0x40], 0x56);

	cpu.iram[0x40] = 0x9A;
	cpu.iram[0] = 0x40;
	cpu_step(&cpu);
	ASSERT_EQ("MOV A,@Ri", cpu.acc, 0x9A);

	cpu.acc = 0x66;
	cpu.iram[0] = 0x40;
	cpu_step(&cpu);
	ASSERT_EQ("MOV @Ri,A", cpu.iram[0x40], 0x66);

	cpu.iram[0x40] = 0x99;
	cpu.iram[0] = 0x40;
	cpu_step(&cpu);
	ASSERT_EQ("MOV direct,@Ri", cpu.iram[0x33], 0x99);

	cpu.iram[0x34] = 0xAA;
	cpu.iram[0] = 0x40;
	cpu_step(&cpu);
	ASSERT_EQ("MOV @Ri,direct", cpu.iram[0x40], 0xAA);

	cpu_step(&cpu);
	ASSERT_EQ("MOV Rn,#imm", cpu.iram[0], 0x78);

	cpu.iram[0] = 0x55;
	cpu_step(&cpu);
	ASSERT_EQ("MOV A,Rn", cpu.acc, 0x55);

	cpu.acc = 0x77;
	cpu_step(&cpu);
	ASSERT_EQ("MOV Rn,A", cpu.iram[0], 0x77);

	cpu_step(&cpu);
	ASSERT_EQ("MOV direct,Rn", cpu.iram[0x32], 0x77);

	cpu.iram[0x32] = 0x88;
	cpu_step(&cpu);
	ASSERT_EQ("MOV Rn,direct", cpu.iram[0], 0x88);
}
