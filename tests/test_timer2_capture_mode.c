#include "test_common.h"
#include "timers.h"

void test_timer2_capture_mode(void)
{
	cpu_t cpu;
	timers_t timers;
	uint8_t code[] = { 0x00 };

	load_code(&cpu, code, sizeof(code));
	timers_init(&timers, &cpu);
	timers_set_profile(&timers, TIMERS_PROFILE_STRICT_8052);

	cpu_write_direct(&cpu, 0xC8, 0x0D);
	cpu_write_direct(&cpu, 0xCD, 0x12);
	cpu_write_direct(&cpu, 0xCC, 0x34);

	ASSERT_TRUE("T2EX capture event", timers_emit_signal_edge(&timers, TIMERS_SIGNAL_T2_EX, TIMERS_EDGE_FALLING, 0));
	timers_tick(&timers, 0);

	ASSERT_EQ("RCAP2H capture", cpu_read_direct(&cpu, 0xCB), 0x12);
	ASSERT_EQ("RCAP2L capture", cpu_read_direct(&cpu, 0xCA), 0x34);
	ASSERT_EQ("T2 EXF2 set", cpu_read_direct(&cpu, 0xC8) & 0x40, 0x40);
}
