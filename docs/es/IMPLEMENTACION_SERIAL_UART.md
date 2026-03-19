# Implementación y uso de serial UART

## 1. Alcance
El módulo UART modela el periférico serial vía SFR y ticks de ciclo:

- SFR principales: `SCON`, `SBUF`, `PCON`, `TCON`, `TMOD`, `TL1`, `TH1`.
- TX/RX asincrónicos por ciclos de bit.
- Soporte de callbacks para TX, frames de 9 bits, cambios de baud y modo 0.

Archivos: `lib/uart.h` y `lib/uart.c`.

## 2. Arquitectura interna
`uart_t` mantiene:

- configuración temporal (`timing_cfg`, `bit_cycles`, `baud`),
- estado de transmisión (`tx_busy`, byte actual, bits restantes),
- estado de recepción (`rx_pending`, byte en tránsito, overrun),
- callbacks de integración con host.

## 3. Cálculo de baud
`uart_update_baud` recalcula al cambiar SFR relevantes.

### 3.1 Modo 0
- Base desde `fosc/64` y `SMOD`.

### 3.2 Modos 1 y 3
- Derivan de Timer1 (`TH1`, `TR1`, `SMOD`).
- `bit_cycles` se lleva a ciclos de máquina para integración con tick CPU.

## 4. Integración con SFR
`uart_attach` instala hooks SFR para interceptar lectura/escritura.

Comportamientos clave:

- Escritura a `SBUF` inicia transmisión (o transferencia síncrona en modo 0).
- `TI` se limpia al iniciar TX y se setea al terminar frame.
- `RI` se setea al completar RX válido.

## 5. Tick de UART
`uart_tick(uart, cycles)` avanza:

- transmisor por acumulador de ciclos,
- receptor por cuenta regresiva de frame.

Cuando termina RX:

- opcionalmente filtra por `SM2` y bit 9,
- escribe en `SBUF`,
- setea `RB8` y `RI`.

## 6. API de uso
### 6.1 Inicialización
1. `uart_init` con `timing_config_t`.
2. `uart_attach` al CPU.
3. Registrar `uart_tick` en hooks de tick CPU.

### 6.2 Callbacks de salida
- `uart_set_tx_callback`: byte completo.
- `uart_set_tx_callback9`: frame con bit9 y baud.
- `uart_set_baud_callback`: notificación de cambio de velocidad.
- `uart_set_mode0_callback`: transferencia en modo 0.

### 6.3 Inyección de entrada (RX)
- `uart_queue_rx_byte` para 8 bits.
- `uart_queue_rx_frame` para 9 bits.

La entrega al firmware ocurre de forma temporizada, no instantánea.

## 7. Ejemplo real en el proyecto
`src/main.c` muestra:

- attach UART,
- callback TX a stdout,
- callback de baud,
- avance por tick hook.

## 8. Ejemplo de código

```c
#include "uart.h"

static void uart_tick_hook(cpu_t *cpu, uint32_t cycles, void *user)
{
	(void)cpu;
	uart_tick((uart_t *)user, cycles);
}

static void on_uart_tx(uint8_t byte, void *user)
{
	(void)user;
	/* Enviar byte al host: consola, socket o archivo */
	(void)byte;
}

void attach_uart(cpu_t *cpu, uart_t *uart, const timing_config_t *timing_cfg)
{
	static cpu_tick_entry_t hooks[1];

	uart_init(uart, timing_cfg);
	uart_attach(cpu, uart);
	uart_set_tx_callback(uart, on_uart_tx, NULL);

	hooks[0].fn = uart_tick_hook;
	hooks[0].user = uart;
	cpu_set_tick_hooks(cpu, hooks, 1);

	uart_queue_rx_byte(uart, 'A');
}
```

## 9. Errores comunes y diagnóstico

- `bit_cycles = 0`: baud no configurado o Timer1 no habilitado.
- RX rechazado: `rx_pending` ocupado (overrun).
- Firmware no ve datos: `REN` deshabilitado o `SM2` filtrando frames.

## 10. Recomendaciones

- Mantener una sola fuente de tiempo por todo el emulador.
- Inyectar RX desde cola/event loop, no directo dentro de ISR pesada.
- Loggear cambios de baud en integración de hardware real.
