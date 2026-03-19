# MCS-51 Runtime Compatibility Layer

## Overview

This project implements a **binary compatibility runtime for MCS-51 (8051) firmware** that allows legacy binaries to run on modern microcontrollers such as STM32 or ESP32.

Instead of recompiling firmware, the runtime executes the original **Intel HEX / binary firmware** through a lightweight CPU emulator and maps hardware access to a modern HAL.

Goal:

Run legacy 8051 firmware **without source code** on modern hardware.

Typical use cases:

* Industrial controller retrofits
* PLC modernization
* Legacy firmware preservation
* Reverse engineering
* Firmware testing

---

# High-Level Architecture

```
+-----------------------------+
| 8051 Firmware (.hex/.bin)   |
+-----------------------------+

            ↓

+-----------------------------+
| 8051 CPU Runtime            |
|                             |
| • instruction decoder       |
| • instruction execution     |
| • registers / flags         |
| • program counter           |
+-----------------------------+

            ↓

+-----------------------------+
| Memory Model                |
|                             |
| • code memory               |
| • internal RAM              |
| • SFR registers             |
+-----------------------------+

            ↓

+-----------------------------+
| Peripheral Translation HAL  |
|                             |
| • GPIO ports                |
| • timers                    |
| • UART                      |
+-----------------------------+

            ↓

+-----------------------------+
| Modern MCU Hardware         |
| (STM32 / ESP32 / host)      |
+-----------------------------+
```

---

# Design Goals

Primary goals:

* Small and portable
* Minimal dependencies
* Suitable for bare-metal environments
* Deterministic execution
* Easy to extend for new peripherals

Secondary goals:

* Host-side debugging
* Firmware tracing
* Instruction statistics
* Reverse engineering support

---

# CPU Model

The runtime implements the core registers of the MCS-51 architecture.

Registers:

```
ACC   accumulator
B     secondary register
PSW   program status word
SP    stack pointer
DPTR  data pointer
PC    program counter
```

General purpose registers are stored in internal RAM.

---

# Memory Model

The runtime implements the standard MCS-51 memory spaces.

Code memory:

```
uint8_t code[65536];
```

Internal RAM:

```
uint8_t iram[256];
```

Special Function Registers:

```
uint8_t sfr[128];
```

SFR accesses must be intercepted to map peripherals.

---

# Instruction Execution Model

Execution is based on a **256-entry opcode table**.

```
typedef struct
{
    void (*exec)(cpu_t *cpu);
    uint8_t size;
    const char *mnemonic;
} instr_t;
```

The CPU fetch-decode-execute cycle:

```
op = code[PC++]

instr = instr_table[op]

instr.exec(cpu)
```

Instruction operands are fetched from code memory using PC.

---

# Instruction Definition DSL

Instructions are defined using an **X-macro table** to ensure consistency across:

* opcode table
* disassembler
* instruction metadata

Example:

```
#define INSTRUCTION_LIST(X) \
X(0x00, 1, NOP,   "NOP") \
X(0x04, 1, INC_A, "INC A") \
X(0x74, 2, MOV_A_IMM, "MOV A,#imm")
```

From this table the project generates:

* instruction table
* disassembler
* opcode enumeration

---

# Code Fetch

Program bytes are fetched sequentially from code memory.

```
uint8_t code_fetch(cpu_t *cpu)
{
    return code[cpu->pc++];
}
```

Operand fetch helpers:

```
uint8_t fetch8(cpu_t *cpu)
uint16_t fetch16(cpu_t *cpu)
```

---

# Peripheral Abstraction Layer

8051 firmware interacts with hardware via **SFR registers**.

Example ports:

```
P0 = 0x80
P1 = 0x90
P2 = 0xA0
P3 = 0xB0
```

SFR writes are intercepted:

```
void sfr_write(uint8_t addr, uint8_t value)
{
    sfr[addr] = value;

    switch(addr)
    {
        case 0x90:
            hal_gpio_write_port1(value);
            break;
    }
}
```

The HAL layer maps these operations to the target MCU.

---

# Hardware Mapping

Example mapping:

```
8051 P1.0 → STM32 GPIOA5
8051 P1.1 → STM32 GPIOA6
```

Configuration may later be provided via JSON or compile-time mapping tables.

---

# Firmware Loader

Firmware is loaded from **Intel HEX** format.

Steps:

1. Parse record
2. Extract address
3. Copy data into code memory

Example:

```
:10010000214601360121470136007EFE09D2190140
```

---

# Execution Modes

Host mode:

```
gcc build
```

Used for:

* debugging
* tracing
* firmware analysis

Embedded mode:

```
STM32 / ESP32
```

Used for:

* running firmware on hardware

---

# Optional Debug Features

Planned debugging features:

* instruction trace
* memory watchpoints
* SFR access log
* execution statistics

Example trace:

```
00001234  MOV A,#0x55
00001236  MOV P1,A
```

---

# Project Structure

```
mcs51-runtime/

cpu.h
cpu.c

instr_table.c
instr_impl.c

hex_loader.c

hal/
  hal_gpio.c
  hal_uart.c

main.c
```

---

# Minimum Instruction Set (MVP)

To support simple firmware the first implementation should include:

```
NOP
MOV variants
INC
DEC
ADD
SJMP
AJMP
LJMP
SETB
CLR
PUSH
POP
```

This covers most simple firmware loops.

---

# Example Firmware Flow

Firmware toggles a GPIO:

```
SETB P1.0
ACALL delay
CLR P1.0
SJMP loop
```

Runtime mapping:

```
SETB P1.0 → HAL_GPIO_WritePin()
```

Result:

LED blinking on STM32 or ESP32.

---

# Future Extensions

Possible future capabilities:

* timer emulation
* UART emulation
* interrupt controller
* cycle-accurate mode
* firmware instrumentation
* PLC runtime compatibility

---

# Project Vision

Provide a **lightweight runtime capable of executing legacy MCS-51 firmware on modern hardware without modification**.

This enables industrial firmware reuse, system retrofits, and binary firmware analysis.
