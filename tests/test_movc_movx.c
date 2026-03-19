#include "test_common.h"

void test_movc_movx(void)
{
	cpu_t cpu;
	uint8_t code[] = { 0x93, 0xE0, 0xF0, 0xE2, 0xF2 };
	load_code(&cpu, code, sizeof(code));
	test_mem_t *mem = get_mem(&cpu);
	cpu.dptr = 0x0100;
	cpu.acc = 0x02;
	mem->code[0x0102] = 0xAA;
	cpu_step(&cpu);
	ASSERT_EQ("MOVC A,@A+DPTR", cpu.acc, 0xAA);

	cpu.dptr = 0x0200;
	mem->xdata[0x0200] = 0xCC;
	cpu_step(&cpu);
	ASSERT_EQ("MOVX A,@DPTR", cpu.acc, 0xCC);

	cpu.acc = 0xDD;
	cpu.dptr = 0x0200;
	cpu_step(&cpu);
	ASSERT_EQ("MOVX @DPTR,A", mem->xdata[0x0200], 0xDD);

	cpu.iram[0] = 0x10;
	mem->xdata[0x10] = 0xEE;
	cpu_step(&cpu);
	ASSERT_EQ("MOVX A,@Ri", cpu.acc, 0xEE);

	cpu.acc = 0x99;
	cpu.iram[0] = 0x10;
	cpu_step(&cpu);
	ASSERT_EQ("MOVX @Ri,A", mem->xdata[0x10], 0x99);
}
