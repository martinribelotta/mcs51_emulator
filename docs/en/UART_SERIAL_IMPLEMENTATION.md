# UART Serial Implementation and Usage

## 1. Scope
The UART module models the serial peripheral through SFR hooks and cycle ticks:

- Main SFRs: `SCON`, `SBUF`, `PCON`, `TCON`, `TMOD`, `TL1`, `TH1`.
- Asynchronous TX/RX using bit-cycle timing.
- Host integration callbacks for TX bytes, 9-bit frames, baud-rate updates, and mode-0 transfers.

Files: `lib/uart.h` and `lib/uart.c`.

## 2. Internal architecture
`uart_t` tracks:

- timing configuration (`timing_cfg`, `bit_cycles`, `baud`),
- TX state (`tx_busy`, current byte, remaining bits),
- RX state (`rx_pending`, in-flight byte, overrun),
- host callback function pointers.

## 3. Baud-rate calculation
`uart_update_baud` recalculates when relevant SFRs change.

### 3.1 Mode 0
- Base from `fosc / 64`, adjusted by `SMOD`.

### 3.2 Modes 1 and 3
- Derived from Timer1 (`TH1`, `TR1`, `SMOD`).
- Converted to machine-cycle `bit_cycles` for CPU tick integration.

## 4. SFR integration
`uart_attach` installs SFR hooks for UART-related registers.

Key behavior:

- Writing `SBUF` starts TX (or synchronous mode-0 exchange).
- `TI` is cleared on TX start and set on frame completion.
- `RI` is set when valid RX completes.

## 5. UART tick behavior
`uart_tick(uart, cycles)` advances:

- transmitter via cycle accumulator,
- receiver via frame countdown.

On RX completion:

- optionally filters by `SM2` and 9th-bit value,
- writes payload to `SBUF`,
- sets `RB8` and `RI`.

## 6. Usage API
### 6.1 Initialization
1. `uart_init` with `timing_config_t`.
2. `uart_attach` to CPU.
3. Register `uart_tick` as CPU tick hook.

### 6.2 TX callbacks
- `uart_set_tx_callback`: byte-level callback.
- `uart_set_tx_callback9`: frame callback with bit9 and baud.
- `uart_set_baud_callback`: baud-change callback.
- `uart_set_mode0_callback`: mode-0 transfer callback.

### 6.3 RX injection
- `uart_queue_rx_byte` for 8-bit input.
- `uart_queue_rx_frame` for 9-bit input.

Delivery to firmware is timed, not instantaneous.

## 7. Working project example
`src/main.c` demonstrates:

- UART attach,
- TX callback to stdout,
- baud-change callback,
- progression via tick hook.

## 8. Common issues and diagnostics

- `bit_cycles = 0`: baud not configured or Timer1 not enabled.
- RX rejected: `rx_pending` already occupied (overrun path).
- Firmware not receiving: `REN` disabled or `SM2` filtering active.

## 9. Recommendations

- Keep one authoritative time source across the emulator.
- Inject RX through queue/event-loop logic rather than heavy ISR work.
- Log baud transitions when integrating with real hardware links.
