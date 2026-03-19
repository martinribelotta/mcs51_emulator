#include "test_common.h"

void test_movc_pc(void)
{
	cpu_t cpu;
	uint8_t code[] = { 0x83, 0x00, 0x55 };
	load_code(&cpu, code, sizeof(code));
	cpu.acc = 0x01;
	cpu_step(&cpu);
	ASSERT_EQ("MOVC A,@A+PC", cpu.acc, 0x55);
}
