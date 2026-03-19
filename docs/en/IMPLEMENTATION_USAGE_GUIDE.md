# Runtime Implementation and Usage Guide

This index organizes the technical documentation by subsystem.

## Documents

1. CPU
   - `docs/en/CPU_IMPLEMENTATION.md`

2. CODE/XDATA Memory
   - `docs/en/CODE_XDATA_MEMORY_IMPLEMENTATION.md`

3. Register and Memory Hooks
   - `docs/en/REGISTER_MEMORY_HOOKS_IMPLEMENTATION.md`

4. UART Serial
   - `docs/en/UART_SERIAL_IMPLEMENTATION.md`

5. Timers
   - `docs/en/TIMERS_IMPLEMENTATION.md`

6. GPIO / Ports
   - `docs/en/GPIO_PORTS_IMPLEMENTATION.md`

## Recommended reading order

### A. Architecture understanding
1) CPU
2) CODE/XDATA Memory
3) Hooks

### B. Peripheral integration
4) UART
5) Timers
6) GPIO

### C. Porting to another MCU
- Read Hooks + Timers + GPIO together.
- Define a HAL layer that translates host physical events into emulator logical signals.

## Minimal integration flow

1. Initialize CPU.
2. Attach CODE/XDATA memory map.
3. Attach UART, Timers, and GPIO via SFR hooks.
4. Register peripheral tick hooks.
5. Load firmware.
6. Execute with `cpu_run` or `cpu_run_timed`.

## Notes

- `src/main.c` is a complete, working wiring reference.
- `tests/test_runner.c` provides useful regression tests.
