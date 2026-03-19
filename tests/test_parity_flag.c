#include "test_common.h"

static uint8_t parity_bit(uint8_t acc)
{
	uint8_t v = acc;
	v ^= (uint8_t)(v >> 4);
	v ^= (uint8_t)(v >> 2);
	v ^= (uint8_t)(v >> 1);
	return (uint8_t)(v & 1u);
}

void test_parity_flag(void)
{
	cpu_t cpu;
	uint8_t code[] = { 0x74, 0x00, 0x74, 0x01, 0x74, 0xFF };
	load_code(&cpu, code, sizeof(code));

	cpu_step(&cpu);
	ASSERT_EQ("P for A=00", cpu.psw & 0x01, parity_bit(cpu.acc));

	cpu_step(&cpu);
	ASSERT_EQ("P for A=01", cpu.psw & 0x01, parity_bit(cpu.acc));

	cpu_step(&cpu);
	ASSERT_EQ("P for A=FF", cpu.psw & 0x01, parity_bit(cpu.acc));
}
