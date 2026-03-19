#include "test_common.h"
#include "timers.h"

void test_timer0_counter_edges(void)
{
	cpu_t cpu;
	timers_t timers;
	uint8_t code[] = { 0x00 };

	load_code(&cpu, code, sizeof(code));
	timers_init(&timers, &cpu);
	cpu_write_direct(&cpu, 0x89, 0x05);
	cpu_write_direct(&cpu, 0x88, 0x10);

	timers_route_input(&timers, 0, true, TIMERS_SIGNAL_T0_CLK, TIMERS_EDGE_RISING);
	ASSERT_TRUE("T0 seed low", timers_input_sample(&timers, 0, false, 0));
	ASSERT_TRUE("T0 first rise", timers_input_sample(&timers, 0, true, 1));
	ASSERT_TRUE("T0 fall", timers_input_sample(&timers, 0, false, 2));
	ASSERT_TRUE("T0 second rise", timers_input_sample(&timers, 0, true, 3));

	timers_tick(&timers, 3);

	uint16_t t0 = (uint16_t)(((uint16_t)cpu_read_direct(&cpu, 0x8C) << 8) | cpu_read_direct(&cpu, 0x8A));
	ASSERT_EQ("T0 counter on rising edges", t0, 0x0002);
}
