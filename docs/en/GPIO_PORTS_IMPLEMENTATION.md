# GPIO Ports Implementation and Usage (P0-P3)

## 1. Scope
The ports module implements `P0..P3` SFR behavior and bridges it to host callbacks.

Files: `lib/ports.h` and `lib/ports.c`.

## 2. Port model
`ports_t` stores:

- CPU reference,
- `latch[4]` (latched output state per port),
- external read callback (`read_cb`),
- write notification callback (`write_cb`).

## 3. Read semantics
On port SFR read:

1. Latch value is taken.
2. Combined with external input (`read_cb`).
3. Pull-up mask is applied per port model.

Current implementation model:

- `P0`: no internal pull-up (`0x00`).
- `P1/P2/P3`: pull-up enabled (`0xFF`).

The resulting value follows quasi-bidirectional port behavior as encoded in `port_read_effective`.

## 4. Write semantics
On `P0..P3` write:

- latch is updated,
- SFR mirror is updated,
- host receives write callback with:
  - driven level,
  - driven-bit mask.

This separates logically high-impedance bits from actively driven bits.

## 5. CPU attachment
`ports_init` installs SFR hooks for `P0`, `P1`, `P2`, `P3`.

It also initializes `latch[]` from current SFR values.

## 6. Usage API
### 6.1 Initialization
1. Define read/write callbacks.
2. Call `ports_init`.
3. If backend changes at runtime, use `ports_set_callbacks`.

### 6.2 Typical integration
- `read_cb`: sample external pin state (host GPIO, waveform source, sensors, etc.).
- `write_cb`: apply port output to hardware or external simulation model.

## 7. Project example
`src/main.c` demonstrates:

- `ports_read_stub` to inject serial RX waveform on P3.
- `ports_write_stdout` to log port output activity.

## 8. Code example

```c
#include <stdio.h>
#include "ports.h"

static uint8_t host_read_pin(uint8_t port, void *user)
{
  const uint8_t *ext_levels = (const uint8_t *)user;
  return ext_levels[port & 0x03u];
}

static void host_write_pin(uint8_t port, uint8_t level, uint8_t mask, void *user)
{
  (void)user;
  /* Forward to logger, host GPIO layer, or external simulation */
  printf("P%u level=0x%02X mask=0x%02X\n", port, level, mask);
}

void attach_ports(cpu_t *cpu, ports_t *ports, uint8_t *ext_levels)
{
  ports_init(ports, cpu, host_read_pin, host_write_pin, ext_levels);
}
```

## 9. Recommendations

- Keep `read_cb` non-blocking.
- Avoid side effects in read callbacks.
- If timer counter-mode edges are needed, derive them in GPIO integration and forward to `timers`.

## 10. Current limitations

- The model is suitable for most classic firmware behavior, but does not emulate fine electrical effects (analog timing, current limits, bus contention).
- For electrical-level fidelity, add a dedicated physical I/O layer on top of this module.
