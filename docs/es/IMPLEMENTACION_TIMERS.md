# Implementación y uso de Timers (T0, T1, T2/T2EX)

## 1. Alcance
El módulo `timers` implementa:

- Timer0 y Timer1 en modos estándar (`TMOD`).
- Timer2 con soporte de overflow, reload y captura externa.
- Entrada por ciclos de CPU y por flancos externos (counter/capture).

Archivos: `lib/timers.h` y `lib/timers.c`.

## 2. Modelo temporal
El módulo mantiene `cycles_total` y procesa dos fuentes de avance:

1. Ciclos provenientes del core (`timers_tick`).
2. Eventos externos encolados con timestamp (flancos de señales).

Esto garantiza orden determinista entre ejecución de instrucciones y señales externas.

## 3. Señales lógicas y ruteo de entradas
### 3.1 Señales soportadas
- `TIMERS_SIGNAL_T0_CLK`
- `TIMERS_SIGNAL_T1_CLK`
- `TIMERS_SIGNAL_T2_CLK`
- `TIMERS_SIGNAL_T2_EX`

### 3.2 Ruteo
`timers_route_input` permite mapear cualquier `input_id` físico del host a una señal lógica y flanco (`rising`/`falling`).

Luego `timers_input_sample`:

- detecta cambio de nivel,
- calcula flanco,
- si coincide con la ruta, encola evento.

## 4. Timer0 y Timer1
### 4.1 Modo timer
En `timers_tick_timer0/1`, si `C/T=0`:

- usan ciclos CPU,
- respetan `TRx` y `GATE`.

### 4.2 Modo counter
Si `C/T=1`, el conteo viene de flancos externos:

- vía `timers_pulse_t0/t1`,
- o vía señales enrutadas y despachadas.

### 4.3 Flags
Setean `TF0/TF1` al overflow según modo.

## 5. Timer2 y T2EX
### 5.1 Conteo principal
`timer2_count_ticks` maneja:

- modo capture/reload según `CPRL2`,
- modo baud por `RCLK/TCLK`,
- seteo de `TF2` según perfil de compatibilidad.

### 5.2 Evento externo T2EX
`timer2_external_event`:

- valida `TR2` y `EXEN2`,
- en capture copia `TH2/TL2` a `RCAP2H/L`,
- en autoreload fuerza reload desde `RCAP2H/L`,
- setea `EXF2`.

## 6. Perfiles de compatibilidad
`timers_profile_t`:

- `TIMERS_PROFILE_STRICT_8052`
- `TIMERS_PROFILE_PRAGMATIC`

Uso principal actual: decidir comportamiento de `TF2` cuando Timer2 está en modo baud (`RCLK/TCLK`).

## 7. Integración práctica
### 7.1 Inicialización
1. `timers_init`.
2. `timers_set_profile` (opcional).
3. Registrar `timers_tick` como hook de tick CPU.

### 7.2 Entradas externas
- Mapear líneas del MCU con `timers_route_input`.
- Desde ISR o polling GPIO, llamar `timers_input_sample` con timestamp coherente.

### 7.3 Interrupciones
El core CPU atiende IRQ de T0/T1/T2 leyendo flags (`TF0/TF1/TF2/EXF2`) en `cpu_check_interrupts`.

## 8. Ejemplo de código

```c
#include "timers.h"

static void timers_tick_hook(cpu_t *cpu, uint32_t cycles, void *user)
{
	(void)cpu;
	timers_tick((timers_t *)user, cycles);
}

void attach_timers(cpu_t *cpu, timers_t *timers)
{
	static cpu_tick_entry_t hooks[1];

	timers_init(timers, cpu);
	timers_set_profile(timers, TIMERS_PROFILE_PRAGMATIC);

	timers_route_input(timers,
					   0,
					   true,
					   TIMERS_SIGNAL_T0_CLK,
					   TIMERS_EDGE_FALLING);

	hooks[0].fn = timers_tick_hook;
	hooks[0].user = timers;
	cpu_set_tick_hooks(cpu, hooks, 1);
}

void on_gpio_sample(timers_t *timers, bool level, uint64_t timestamp)
{
	timers_input_sample(timers, 0, level, timestamp);
}
```

## 9. Validación en el proyecto
Los tests en `tests/test_runner.c` cubren:

- conteo por flancos de T0,
- captura T2 por T2EX,
- reload externo T2,
- perfil strict/pragmatic,
- interrupción T2 por `EXF2`.

## 10. Recomendaciones de portabilidad

- Mantener timestamps monotónicos.
- Minimizar trabajo en ISR: encolar evento y procesar en loop del emulador.
- Si hay jitter alto, usar captura hardware del MCU para mejorar precisión de flanco.
