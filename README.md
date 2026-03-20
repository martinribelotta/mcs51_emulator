# MCS-51 Emulator

A lightweight emulator for the Intel 8051 (MCS-51) architecture that allows you to run legacy firmware without needing the original source code.

## Purpose

This project implements a binary compatibility layer for MCS-51 that enables execution of original binaries (Intel HEX or binary format) on modern hardware like STM32 or ESP32, without needing to recompile the firmware.

The emulator takes your original .hex or binary firmware, decodes the 8051 instruction set, and executes it faithfully with realistic timing and peripheral simulation.

## When You'd Use This

- Industrial equipment retrofitting: modernize legacy controllers while maintaining the original firmware
- PLC modernization: migrate systems to modern hardware when original equipment is hard to find
- Firmware archaeology: analyze binaries without source code access
- Testing and validation: verify firmware behavior before deploying to target hardware
- Hardware migration: move working firmware to new platforms without recompilation

## Architecture

```
├─ 8051 Firmware (.hex/.bin)
├─ CPU Emulator (decodes & runs 8051 instructions)
├─ Memory layer (code, RAM, SFR registers)
├─ Peripheral simulation (UART, timers, GPIO)
└─ Real hardware (or host machine)
```

The system consists of several interdependent layers:

- CPU core: handles instruction decoding and execution, manages registers, flags, and program counter
- Memory: simulates the 8051's address spaces including code ROM, internal RAM, and special function registers
- Peripherals: simulate UART, timers, and I/O ports allowing firmware to interact with virtual hardware
- Timing: tracks execution cycles to maintain timing accuracy for timing-sensitive code

## Project Structure

```
mcs51-emulator/
├── lib/                      # Core emulator library
│   ├── cpu.c/.h             # CPU core - instruction execution
│   ├── decoder.c/.h         # Instruction decoder
│   ├── instr_impl.c/.h      # Instruction implementations
│   ├── mem_map.c/.h         # Memory address space mapping
│   ├── timers.c/.h          # Timer simulation (Timer0/1/2)
│   ├── uart.c/.h            # UART serial port simulation
│   ├── ports.c/.h           # GPIO port simulation
│   └── timing.c/.h          # Cycle counting and timing control
├── src/main.c               # Command-line emulator application
├── tests/                   # Comprehensive unit test suite
├── mc51code/                # Example 8051 assembly programs
├── docs/                    # Documentation (Spanish and English)
└── build/                   # CMake build output
```

The core implementation files (cpu.c, decoder.c, instr_impl.c) handle the emulation logic. The remaining components provide memory management, peripheral simulation, and timing control.

## Using the Emulator

The main.c program provides a command-line interface to run firmware binaries. It performs the following steps:

1. Initializes CPU with 64KB code memory and 64KB data memory
2. Attaches peripheral simulations (UART, timers, GPIO)
3. Loads the firmware .hex file into code memory
4. Configures execution hooks for cycle-accurate timing
5. Executes the firmware until halt, step limit, or error

### Command-Line Usage

Load and run firmware with full instruction tracing:
```bash
./mcs51_emulator firmware.hex 100000 --trace
```

Run firmware for up to 50000 steps:
```bash
./mcs51_emulator firmware.hex 50000
```

Run firmware until it halts:
```bash
./mcs51_emulator firmware.hex
```

### Output Examples

With `--trace` enabled, each instruction is logged with register state:
```
0000  00  NOP                ACC=00 PSW=00 SP=07 DPTR=0000
0001  00  NOP                ACC=00 PSW=00 SP=07 DPTR=0000
0002  74  MOV A,#20          ACC=20 PSW=00 SP=07 DPTR=0000
```

UART output from the firmware:
```
UART baud: 9600
Hello from 8051!
```

GPIO port writes:
```
P1: level=0x55 mask=0xFF
```

Performance metrics at completion:
```
Metrics: steps=100000 wall=50.123 ms emu=9.050 ms speed=1995000.00 steps/s
```

## Building

Prerequisites:
- CMake 3.10 or later
- C11-compatible compiler (gcc, clang)
- Make or Ninja

Build steps:
```bash
mkdir -p build && cd build
cmake ..
make
```

This produces the `mcs51_emulator` executable in the build directory.

### Running Tests

Execute the test suite to verify correct operation:
```bash
ctest --output-on-failure
```

The tests cover instruction implementations, memory access patterns, timer behavior, and edge cases. All tests passing indicates the emulator is functioning correctly.

## Debugging

The emulator provides several tools for debugging firmware:

- Instruction tracing (`--trace`): logs each executed instruction with corresponding CPU state
- Register inspection: trace output displays ACC, PSW, SP, and DPTR at each step
- Performance metrics: execution speed relative to wall clock time helps identify bottlenecks

## Additional Resources

- [ARCHITECTURE.md](ARCHITECTURE.md) - technical implementation details and design rationale
- [docs/es/](docs/es/) - Spanish documentation
- [docs/en/](docs/en/) - English documentation
