#include "test_common.h"
#include "timers.h"

void test_timer2_baud_profile_tf2(void)
{
	cpu_t cpu;
	timers_t timers;
	uint8_t code[] = { 0x00 };

	load_code(&cpu, code, sizeof(code));
	timers_init(&timers, &cpu);
	timers_set_profile(&timers, TIMERS_PROFILE_STRICT_8052);
	cpu_write_direct(&cpu, 0xC8, 0x24);
	cpu_write_direct(&cpu, 0xCD, 0xFF);
	cpu_write_direct(&cpu, 0xCC, 0xFF);

	timers_tick(&timers, 1);
	ASSERT_EQ("strict baud TF2", cpu_read_direct(&cpu, 0xC8) & 0x80, 0x00);

	load_code(&cpu, code, sizeof(code));
	timers_init(&timers, &cpu);
	timers_set_profile(&timers, TIMERS_PROFILE_PRAGMATIC);
	cpu_write_direct(&cpu, 0xC8, 0x24);
	cpu_write_direct(&cpu, 0xCD, 0xFF);
	cpu_write_direct(&cpu, 0xCC, 0xFF);

	timers_tick(&timers, 1);
	ASSERT_EQ("pragmatic baud TF2", cpu_read_direct(&cpu, 0xC8) & 0x80, 0x80);
}
