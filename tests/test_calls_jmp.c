#include "test_common.h"

void test_calls_jmp(void)
{
	cpu_t cpu;
	uint8_t code_acall[] = { 0x11, 0x03, 0x00, 0x22 };
	load_code(&cpu, code_acall, sizeof(code_acall));
	cpu_step(&cpu);
	ASSERT_EQ("ACALL", cpu.pc, 0x0003);
	cpu_step(&cpu);
	ASSERT_EQ("RET", cpu.pc, 0x0002);

	uint8_t code_lcall[] = { 0x12, 0x00, 0x05, 0x00, 0x00, 0x22 };
	load_code(&cpu, code_lcall, sizeof(code_lcall));
	cpu_step(&cpu);
	ASSERT_EQ("LCALL", cpu.pc, 0x0005);
	cpu_step(&cpu);
	ASSERT_EQ("RET after LCALL", cpu.pc, 0x0003);

	uint8_t code_jmp[] = { 0x73 };
	load_code(&cpu, code_jmp, sizeof(code_jmp));
	cpu.acc = 0x02;
	cpu.dptr = 0x0010;
	cpu_step(&cpu);
	ASSERT_EQ("JMP @A+DPTR", cpu.pc, 0x0012);
}
