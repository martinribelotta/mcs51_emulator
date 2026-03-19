#include "test_common.h"

void test_timer2_interrupt_from_exf2(void)
{
	cpu_t cpu;
	uint8_t code[] = { 0x00, 0x00 };

	load_code(&cpu, code, sizeof(code));
	cpu_write_direct(&cpu, 0xA8, 0xA0);
	cpu_write_direct(&cpu, 0xC8, 0x40);

	cpu_poll_interrupts(&cpu);
	ASSERT_EQ("T2 interrupt from EXF2", cpu.pc, 0x002B);
}
