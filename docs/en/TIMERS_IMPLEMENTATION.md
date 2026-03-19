# Timers Implementation and Usage (T0, T1, T2/T2EX)

## 1. Scope
The `timers` module implements:

- Timer0 and Timer1 with standard `TMOD` behavior.
- Timer2 with overflow, reload, and external capture/reload handling.
- Two time sources: CPU cycle ticks and external edge events.

Files: `lib/timers.h` and `lib/timers.c`.

## 2. Time model
The module tracks `cycles_total` and advances from:

1. CPU-provided cycles (`timers_tick`).
2. Externally queued signal-edge events with timestamps.

This preserves deterministic ordering between instruction execution and external signal activity.

## 3. Logical signals and input routing
### 3.1 Supported signals
- `TIMERS_SIGNAL_T0_CLK`
- `TIMERS_SIGNAL_T1_CLK`
- `TIMERS_SIGNAL_T2_CLK`
- `TIMERS_SIGNAL_T2_EX`

### 3.2 Input routing
`timers_route_input` maps any host-side `input_id` to a logical signal and selected edge (`rising`/`falling`).

Then `timers_input_sample`:

- detects level transitions,
- derives edge direction,
- queues an event when route+edge match.

## 4. Timer0 and Timer1
### 4.1 Timer mode
In `timers_tick_timer0/1`, when `C/T = 0`:

- counting uses CPU cycles,
- `TRx` and `GATE` constraints are respected.

### 4.2 Counter mode
When `C/T = 1`, counting is edge-driven:

- via `timers_pulse_t0/t1`,
- or via routed and dispatched logical signal events.

### 4.3 Flags
`TF0/TF1` are set according to mode-specific overflow rules.

## 5. Timer2 and T2EX
### 5.1 Main counting path
`timer2_count_ticks` handles:

- capture vs auto-reload according to `CPRL2`,
- baud-related operation via `RCLK/TCLK`,
- `TF2` policy through selected compatibility profile.

### 5.2 External T2EX events
`timer2_external_event`:

- validates `TR2` and `EXEN2`,
- in capture mode copies `TH2/TL2` to `RCAP2H/L`,
- in auto-reload mode forces reload from `RCAP2H/L`,
- sets `EXF2`.

## 6. Compatibility profiles
`timers_profile_t`:

- `TIMERS_PROFILE_STRICT_8052`
- `TIMERS_PROFILE_PRAGMATIC`

Current profile impact: `TF2` behavior when Timer2 runs in baud-related modes (`RCLK/TCLK`).

## 7. Practical integration
### 7.1 Initialization
1. `timers_init`.
2. Optional `timers_set_profile`.
3. Register `timers_tick` as CPU tick hook.

### 7.2 External inputs
- Map MCU input lines with `timers_route_input`.
- From ISR or GPIO polling, call `timers_input_sample` with coherent timestamps.

### 7.3 Interrupts
CPU interrupt controller services T0/T1/T2 using `TF0/TF1/TF2/EXF2` flags in `cpu_check_interrupts`.

## 8. Code example

```c
#include "timers.h"

static void timers_tick_hook(cpu_t *cpu, uint32_t cycles, void *user)
{
	(void)cpu;
	timers_tick((timers_t *)user, cycles);
}

void attach_timers(cpu_t *cpu, timers_t *timers)
{
	static cpu_tick_entry_t hooks[1];

	timers_init(timers, cpu);
	timers_set_profile(timers, TIMERS_PROFILE_PRAGMATIC);

	timers_route_input(timers,
					   0,
					   true,
					   TIMERS_SIGNAL_T0_CLK,
					   TIMERS_EDGE_FALLING);

	hooks[0].fn = timers_tick_hook;
	hooks[0].user = timers;
	cpu_set_tick_hooks(cpu, hooks, 1);
}

void on_gpio_sample(timers_t *timers, bool level, uint64_t timestamp)
{
	timers_input_sample(timers, 0, level, timestamp);
}
```

## 9. Project validation coverage
`tests/test_runner.c` validates:

- T0 edge-driven counting,
- T2 capture via T2EX,
- T2 external reload,
- strict vs pragmatic profile behavior,
- T2 interrupt dispatch from `EXF2`.

## 10. Portability recommendations

- Use monotonic timestamps.
- Keep ISR work minimal: queue events and process in emulator loop.
- For high-jitter platforms, prefer hardware input capture to improve edge accuracy.
