# MCS51 Emulator Core

Portable MCS-51 (8051) CPU emulator written in C.

## Overview

This repository contains a standalone emulator core for the MCS-51 architecture.

It implements:

- CPU execution engine
- Instruction decoding
- Memory model (code, IRAM, SFR, XRAM)
- Cycle-based timing
- Instruction tracing

The core is designed to be embedded into other systems and extended with platform-specific integrations.

## Features

- Full 8051 CPU emulation
- Cycle-accurate timing model
- Interceptable memory access
- SFR and XRAM hooks
- Instruction trace support
- No platform dependencies

## Design Goals

- Portable C implementation
- No dynamic allocation
- Deterministic execution
- Easy integration into embedded systems

## Architecture

The emulator is structured as:

- CPU state structure
- Instruction dispatch table
- Memory access callbacks
- External hooks for peripherals

All hardware-specific behavior is delegated to the host application.

## Intended Usage

This core is not a full system emulator.

It is intended to be used as a building block for:

- Firmware execution runtimes
- Hardware compatibility layers
- Embedded simulation environments
- Reverse engineering tools

## Integration Example

See the STM32 integration example:

mcs51_emulator_h750

This demonstrates how to:

- map SFR registers to hardware peripherals
- run firmware on a real microcontroller
- preserve timing and behavior

## Status

The core is stable and used in a working STM32 runtime that executes real firmware.

## License

[tu licencia]
