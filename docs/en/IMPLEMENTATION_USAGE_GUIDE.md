# Runtime Implementation and Usage Guide

This index organizes the technical documentation by subsystem.

## Documents

1. CPU
   - [CPU implementation](CPU_IMPLEMENTATION.md)

2. CODE/XDATA Memory
   - [CODE/XDATA memory implementation](CODE_XDATA_MEMORY_IMPLEMENTATION.md)

3. Register and Memory Hooks
   - [Register and memory hooks implementation](REGISTER_MEMORY_HOOKS_IMPLEMENTATION.md)

4. UART Serial
   - [UART serial implementation](UART_SERIAL_IMPLEMENTATION.md)

5. Timers
   - [Timers implementation](TIMERS_IMPLEMENTATION.md)

6. GPIO / Ports
   - [GPIO ports implementation](GPIO_PORTS_IMPLEMENTATION.md)

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

- The [main integration source](../../src/main.c) is a complete, working wiring reference.
- The [regression test suite](../../tests/test_runner.c) provides useful regression tests.

## Cross links

- Spanish guide: [Guía en español](../es/GUIA_IMPLEMENTACION_USO.md)
- CPU ↔ Memory: [CPU implementation and usage](CPU_IMPLEMENTATION.md) and [CODE/XDATA memory implementation](CODE_XDATA_MEMORY_IMPLEMENTATION.md)
- Hooks ↔ Peripherals: [Register and memory hooks implementation](REGISTER_MEMORY_HOOKS_IMPLEMENTATION.md), [UART serial implementation](UART_SERIAL_IMPLEMENTATION.md), [Timers implementation](TIMERS_IMPLEMENTATION.md), [GPIO ports implementation](GPIO_PORTS_IMPLEMENTATION.md)
