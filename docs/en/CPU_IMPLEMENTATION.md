# CPU Implementation and Usage

## 1. Scope
This document explains how the MCS-51 CPU core is implemented and how to integrate it in host or embedded environments.

## 2. CPU internal state
`cpu_t` models:

- Main registers: `ACC`, `B`, `PSW`, `SP`, `DPTR`, `PC`.
- Core internal memories: `iram[256]`, `sfr[128]`.
- Execution infrastructure: tracing, memory hooks, tick hooks.
- Interrupt state: ISR nesting, active priority, ISR shadow stack.
- Low-power state: `idle` and `power_down`.

API and layout reference: `lib/cpu.h`.

## 3. Execution cycle
### 3.1 Single step
`cpu_step` performs:

1. Stop-state checks (`halted`, `idle`, `power_down`).
2. Instruction fetch-decode-execute.
3. `PSW` parity update.
4. Interrupt polling.
5. Machine-cycle count return for the executed instruction.

### 3.2 Continuous run
`cpu_run` and `cpu_run_timed`:

- Repeatedly call `cpu_step`.
- Execute per-instruction tick hooks (`cpu_set_tick_hooks`).
- Keep peripheral time advancing during `idle` using 1 virtual cycle per loop.
- In timed mode, synchronize emulated time to wall-clock via `cpu_time_iface_t`.

## 4. Initialization and reset
### 4.1 Base initialization
`cpu_init` copies `CPU_INIT_TEMPLATE`.

The template already installs default SFR mirror hooks for `ACC`, `B`, `PSW`, `SP`, `DPL`, and `DPH`.

### 4.2 Reset
`cpu_reset`:

- Preserves system wiring (SFR hooks, memory hooks, tick hooks, and associated user pointers).
- Reinitializes registers and SFR power-up values.
- Clears execution and interrupt runtime state.

This allows reusing the same board wiring across multiple resets.

## 5. Interrupts
The internal controller supports:

- Sources: `INT0`, `T0`, `INT1`, `T1`, `SERIAL`, `T2`.
- Low/high priorities through `IP`.
- ISR nesting bounded by internal stack (`isr_stack`).
- Standard MCS-51 vectors (`0x0003`, `0x000B`, `0x0013`, `0x001B`, `0x0023`, `0x002B`).

Implementation notes:

- `T0` and `T1` clear `TF0/TF1` upon ISR entry.
- `T2` interrupt is accepted from `TF2` or `EXF2`.
- `cpu_on_reti` restores previous ISR priority context.

## 6. Low-power behavior
### 6.1 IDLE
- `cpu_step` returns 0.
- `cpu_run`/`cpu_run_timed` still advance peripherals by 1 virtual cycle.
- Wake-up can occur via enabled interrupts.

### 6.2 POWER DOWN
- Instruction execution is stopped.
- Interrupt service is blocked until external wake (`cpu_wake`).

## 7. Recommended integration
Minimal host checklist:

1. `cpu_init`.
2. Attach memory map (`mem_map_attach`).
3. Attach peripherals through SFR and/or tick hooks.
4. Load firmware into CODE.
5. Run with `cpu_run` or `cpu_run_timed`.

Working integration example: the [main integration source](../../src/main.c).

## 8. Code example

```c
#include "cpu.h"
#include "timers.h"

static void timers_tick_hook(cpu_t *cpu, uint32_t cycles, void *user)
{
	(void)cpu;
	timers_tick((timers_t *)user, cycles);
}

void run_firmware(cpu_t *cpu, timers_t *timers)
{
	cpu_tick_entry_t hooks[] = {
		{ .fn = timers_tick_hook, .user = timers },
	};

	cpu_init(cpu);
	timers_init(timers, cpu);
	cpu_set_tick_hooks(cpu, hooks, MCS51_ARRAY_LEN(hooks));

	cpu_run(cpu, 100000);
}
```

## 9. Best practices

- Use `cpu_step` for instruction-level debugging.
- Use `cpu_run_timed` only when real-time pacing is required.
- Keep peripherals in tick hooks and decoupled from the decoder.
- Avoid direct external writes to `cpu->sfr[]`; prefer APIs and hooks.

## 10. Related links

- General guide: [Runtime implementation and usage guide](IMPLEMENTATION_USAGE_GUIDE.md)
- CODE/XDATA Memory: [CODE/XDATA memory implementation](CODE_XDATA_MEMORY_IMPLEMENTATION.md)
- Hooks and peripherals: [Register and memory hooks implementation](REGISTER_MEMORY_HOOKS_IMPLEMENTATION.md)
- Spanish version: [Spanish CPU documentation](../es/IMPLEMENTACION_CPU.md)
