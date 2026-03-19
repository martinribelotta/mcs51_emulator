# Register and Memory Hooks Implementation and Usage

## 1. Problem solved by hooks
Hooks allow intercepting core accesses to:

- SFR (special function registers).
- CODE/XDATA external memory.
- Cycle ticks (time progression for peripherals).

This keeps the CPU core decoupled from peripheral and host hardware specifics.

## 2. SFR hooks
### 2.1 API
- `cpu_set_sfr_hook(cpu, addr, read, write, user)`

### 2.2 User pointer semantics
Each hook uses its own `user` pointer configured through `cpu_set_sfr_hook`.

### 2.3 Effective access flow
- `cpu_read_direct` / `cpu_write_direct` detect SFR addresses.
- If a hook exists, callback is invoked.
- Otherwise, the core accesses `cpu->sfr[]` directly.

## 3. External memory hooks
### 3.1 API
- `cpu_set_mem_ops(cpu, ops, user)`

`ops` provides CODE and XDATA callbacks.

### 3.2 Recommended integration
Prefer `mem_map_attach` for region-based wiring, unless special custom behavior is required.

## 4. Tick hooks
### 4.1 API
- `cpu_set_tick_hooks(cpu, hooks, count)`

Each `cpu_tick_entry_t` receives:

- CPU pointer,
- cycles consumed by the instruction,
- hook user context.

### 4.2 Execution timing
In `cpu_run` and `cpu_run_timed`, tick hooks run immediately after `cpu_step` and before the next instruction.

This is the canonical location for:

- timers,
- UART,
- periodic GPIO logic,
- peripheral co-simulation.

## 5. Design patterns
### 5.1 Peripheral module pattern
Each peripheral module should expose:

1. attach function (installs SFR hooks),
2. tick function (advances internal time),
3. host integration callbacks.

UART, Timers, and Ports in this project follow this pattern.

### 5.2 Avoid circular coupling
- Keep each hook focused on its own SFR and local state.
- Cross-peripheral interaction should use tick hooks or explicit signal/event contracts.

## 6. Common errors

- Installing an SFR hook for address `< 0x80` (ignored by design).
- Using invalid/expired user pointers.
- Writing critical SFR bits without preserving expected interrupt-flag semantics.

## 7. Integration checklist

1. Define peripheral state struct.
2. Install required SFR hooks.
3. Register tick hook if cycle-dependent.
4. Validate interrupt interactions.
5. Add a minimal functional test.

## 8. Code example

```c
#include "cpu.h"

typedef struct {
	uint8_t shadow;
} led_dev_t;

static uint8_t led_read(const cpu_t *cpu, uint8_t addr, void *user)
{
	(void)cpu;
	(void)addr;
	return ((led_dev_t *)user)->shadow;
}

static void led_write(cpu_t *cpu, uint8_t addr, uint8_t value, void *user)
{
	(void)cpu;
	(void)addr;
	((led_dev_t *)user)->shadow = value;
}

static void led_tick(cpu_t *cpu, uint32_t cycles, void *user)
{
	(void)cpu;
	(void)cycles;
	(void)user;
}

void attach_led(cpu_t *cpu, led_dev_t *dev)
{
	static cpu_tick_entry_t hooks[1];

	cpu_set_sfr_hook(cpu, 0x90, led_read, led_write, dev); /* P1 */

	hooks[0].fn = led_tick;
	hooks[0].user = dev;
	cpu_set_tick_hooks(cpu, hooks, 1);
}
```
