#include "test_common.h"

void test_addc_subb_flags(void)
{
	cpu_t cpu;
	uint8_t code[] = { 0x34, 0x00, 0x94, 0x00 };
	load_code(&cpu, code, sizeof(code));

	cpu.acc = 0x0F;
	cpu_set_carry(&cpu, true);
	cpu_step(&cpu);
	ASSERT_EQ("ADDC AC", cpu.psw & 0x40, 0x40);

	cpu.acc = 0x00;
	cpu_set_carry(&cpu, true);
	cpu_step(&cpu);
	ASSERT_EQ("SUBB CY", cpu.psw & 0x80, 0x80);
}
