#include "test_common.h"
#include "timers.h"

void test_timer2_external_reload_mode(void)
{
	cpu_t cpu;
	timers_t timers;
	uint8_t code[] = { 0x00 };

	load_code(&cpu, code, sizeof(code));
	timers_init(&timers, &cpu);

	cpu_write_direct(&cpu, 0xC8, 0x0C);
	cpu_write_direct(&cpu, 0xCB, 0xA5);
	cpu_write_direct(&cpu, 0xCA, 0xC3);
	cpu_write_direct(&cpu, 0xCD, 0xFF);
	cpu_write_direct(&cpu, 0xCC, 0xFE);

	ASSERT_TRUE("T2EX reload event", timers_emit_signal_edge(&timers, TIMERS_SIGNAL_T2_EX, TIMERS_EDGE_FALLING, 0));
	timers_tick(&timers, 0);

	ASSERT_EQ("T2 reload TH2", cpu_read_direct(&cpu, 0xCD), 0xA5);
	ASSERT_EQ("T2 reload TL2", cpu_read_direct(&cpu, 0xCC), 0xC3);
	ASSERT_EQ("T2 EXF2 set", cpu_read_direct(&cpu, 0xC8) & 0x40, 0x40);
}
