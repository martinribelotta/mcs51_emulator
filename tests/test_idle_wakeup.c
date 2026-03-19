#include "test_common.h"

void test_idle_wakeup(void)
{
	cpu_t cpu;
	uint8_t code[] = { 0x00, 0x00 };
	load_code(&cpu, code, sizeof(code));
	cpu_write_direct(&cpu, 0xA8, 0x82);
	cpu_write_direct(&cpu, 0x88, 0x20);
	cpu_write_direct(&cpu, 0x87, 0x01);

	uint8_t cycles = cpu_step(&cpu);
	ASSERT_EQ("IDL cpu_step cycles", cycles, 0);
	cpu_poll_interrupts(&cpu);
	ASSERT_EQ("IDL wake vector", cpu.pc, 0x000B);
}
