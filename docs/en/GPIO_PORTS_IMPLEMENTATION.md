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

## 8. Recommendations

- Keep `read_cb` non-blocking.
- Avoid side effects in read callbacks.
- If timer counter-mode edges are needed, derive them in GPIO integration and forward to `timers`.

## 9. Current limitations

- The model is suitable for most classic firmware behavior, but does not emulate fine electrical effects (analog timing, current limits, bus contention).
- For electrical-level fidelity, add a dedicated physical I/O layer on top of this module.
