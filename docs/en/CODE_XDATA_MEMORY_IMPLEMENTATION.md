# CODE/XDATA Memory Implementation and Usage

## 1. Emulator memory model
The core separates standard MCS-51 spaces:

- CODE (program fetch + optional writes).
- XDATA (external data memory).
- IRAM/SFR (internal to `cpu_t`).

This document covers CODE and XDATA, which are connected through callbacks.

## 2. Region-based mapping layer
`mem_map_t` defines CODE and XDATA region tables.

Each `mem_map_region_t` includes:

- `base`: 16-bit start address.
- `size`: byte size.
- `read`: read callback.
- `write`: write callback.
- `user`: backend context pointer.

Implementation: `lib/mem_map.h` and `lib/mem_map.c`.

## 3. Address resolution
Region lookup:

- Iterates linearly over configured regions.
- Checks membership in `[base, base + size)`.
- Uses the first matching valid region.

If no region/callback is present:

- Read returns `0xFF`.
- Write is ignored.

## 4. CPU attachment
`mem_map_attach(cpu, mem)` installs internal `cpu_mem_ops_t` wiring for:

- `cpu_code_read/cpu_code_write`.
- `cpu_xdata_read/cpu_xdata_write`.

After attachment, opcode fetches and MOVX accesses flow through this map.

## 5. Recommended usage pattern
### 5.1 Simple host setup
- Allocate flat 64K arrays for CODE and XDATA.
- Define one region for each memory space.
- Use RAM-like callbacks.

This pattern is implemented in `src/main.c`.

### 5.2 Embedded setup
- CODE can point to flash/ROM or firmware buffers.
- XDATA can map to external SRAM, FRAM, or emulated RAM.
- Multiple regions can be combined (for example ROM + MMIO windows).

## 6. Important rules

- Keep callbacks fast and side-effect controlled (invoked at instruction granularity).
- Avoid heavy side effects in CODE reads.
- For XDATA MMIO ranges, document address contracts explicitly.

## 7. Troubleshooting
Typical symptoms of incorrect memory wiring:

- Invalid opcode halt: likely missing CODE image or wrong mapping.
- Constant `0xFF` reads: region not found or callback missing.
- MOVX not storing data: missing write callback or wrong XDATA mapping.
