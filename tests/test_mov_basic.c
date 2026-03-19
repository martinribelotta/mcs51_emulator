#include "test_common.h"

void test_mov_basic(void)
{
	cpu_t cpu;
	uint8_t code[] = {
		0x74, 0x12,
		0x75, 0x30, 0x34,
		0xE5, 0x30,
		0xF5, 0x31,
		0x85, 0x30, 0x35,
		0x90, 0x12, 0x34,
		0xA2, 0x00,
		0x92, 0x01
	};
	load_code(&cpu, code, sizeof(code));
	cpu.iram[0x30] = 0x34;
	cpu.iram[0x20] = 0x01;

	cpu_step(&cpu);
	ASSERT_EQ("MOV A,#imm", cpu.acc, 0x12);
	cpu_step(&cpu);
	ASSERT_EQ("MOV direct,#imm", cpu.iram[0x30], 0x34);
	cpu_step(&cpu);
	ASSERT_EQ("MOV A,direct", cpu.acc, 0x34);
	cpu_step(&cpu);
	ASSERT_EQ("MOV direct,A", cpu.iram[0x31], 0x34);
	cpu.iram[0x30] = 0x5A;
	cpu_step(&cpu);
	ASSERT_EQ("MOV direct,direct", cpu.iram[0x35], 0x5A);
	cpu_step(&cpu);
	ASSERT_EQ("MOV DPTR,#imm16", cpu.dptr, 0x1234);
	cpu_step(&cpu);
	ASSERT_TRUE("MOV C,bit", cpu_get_carry(&cpu));
	cpu_set_carry(&cpu, true);
	cpu_step(&cpu);
	ASSERT_EQ("MOV bit,C", cpu.iram[0x20] & 0x02, 0x02);
}
