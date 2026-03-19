#include "test_common.h"

void test_djnz_cjne(void)
{
	cpu_t cpu;
	uint8_t code_djnz_d[] = { 0xD5, 0x30, 0x02 };
	load_code(&cpu, code_djnz_d, sizeof(code_djnz_d));
	cpu.iram[0x30] = 2;
	cpu_step(&cpu);
	ASSERT_EQ("DJNZ direct", cpu.pc, 5);

	uint8_t code_djnz_r[] = { 0xD8, 0x02 };
	load_code(&cpu, code_djnz_r, sizeof(code_djnz_r));
	cpu.iram[0] = 2;
	cpu_step(&cpu);
	ASSERT_EQ("DJNZ Rn", cpu.pc, 4);

	uint8_t code_cjne_a_imm[] = { 0xB4, 0x01, 0x02 };
	load_code(&cpu, code_cjne_a_imm, sizeof(code_cjne_a_imm));
	cpu.acc = 0;
	cpu_step(&cpu);
	ASSERT_EQ("CJNE A,#imm", cpu.pc, 5);

	uint8_t code_cjne_a_dir[] = { 0xB5, 0x31, 0x02 };
	load_code(&cpu, code_cjne_a_dir, sizeof(code_cjne_a_dir));
	cpu.acc = 2;
	cpu.iram[0x31] = 3;
	cpu_step(&cpu);
	ASSERT_EQ("CJNE A,direct", cpu.pc, 5);

	uint8_t code_cjne_ri[] = { 0xB6, 0x02, 0x02 };
	load_code(&cpu, code_cjne_ri, sizeof(code_cjne_ri));
	cpu.iram[0] = 0x40;
	cpu.iram[0x40] = 1;
	cpu_step(&cpu);
	ASSERT_EQ("CJNE @Ri,#imm", cpu.pc, 5);

	uint8_t code_cjne_rn[] = { 0xB8, 0x02, 0x02 };
	load_code(&cpu, code_cjne_rn, sizeof(code_cjne_rn));
	cpu.iram[0] = 1;
	cpu_step(&cpu);
	ASSERT_EQ("CJNE Rn,#imm", cpu.pc, 5);
}
