# Implementación y uso de CPU

## 1. Objetivo
Este documento describe cómo está implementado el núcleo de CPU del emulador MCS-51 y cómo usarlo desde una aplicación host o embebida.

## 2. Estado interno del CPU
La estructura `cpu_t` modela:

- Registros principales: `ACC`, `B`, `PSW`, `SP`, `DPTR`, `PC`.
- Memorias internas del core: `iram[256]`, `sfr[128]`.
- Infraestructura de ejecución: trazas, hooks de memoria, hooks de tick.
- Estado de interrupciones: nesting ISR, prioridad activa, stack interno de ISR.
- Estado de bajo consumo: `idle` y `power_down`.

Referencia de API y layout: `lib/cpu.h`.

## 3. Ciclo de ejecución
### 3.1 Paso único
`cpu_step` realiza:

1. Verifica estados de parada (`halted`, `idle`, `power_down`).
2. Fetch-decode-execute de una instrucción.
3. Actualización de paridad de `PSW`.
4. Sondeo de interrupciones.
5. Devuelve ciclos de máquina consumidos por la instrucción.

### 3.2 Bucle continuo
`cpu_run` y `cpu_run_timed`:

- Llaman repetidamente a `cpu_step`.
- Ejecutan hooks de tick por instrucción (`cpu_set_tick_hooks`).
- Mantienen avance temporal durante `idle` usando 1 ciclo por iteración.
- En modo timed, sincronizan tiempo emulado contra reloj de pared con `cpu_time_iface_t`.

## 4. Inicialización y reset
### 4.1 Inicialización base
`cpu_init` copia `CPU_INIT_TEMPLATE`.

El template ya deja instalados hooks SFR para registros espejo importantes (`ACC`, `B`, `PSW`, `SP`, `DPL`, `DPH`).

### 4.2 Reset
`cpu_reset`:

- Conserva wiring del sistema (hooks SFR, hooks de memoria, hooks de tick y usuario asociado).
- Reinicia registros y SFR a valores de arranque.
- Limpia estado de ejecución e interrupciones.

Esto permite reusar el mismo “board setup” entre resets sin reinstalar toda la plataforma.

## 5. Interrupciones
El controlador interno implementa:

- Fuentes: `INT0`, `T0`, `INT1`, `T1`, `SERIAL`, `T2`.
- Prioridades baja/alta por `IP`.
- Anidación de ISR limitada por stack interno (`isr_stack`).
- Vectores clásicos MCS-51 (`0x0003`, `0x000B`, `0x0013`, `0x001B`, `0x0023`, `0x002B`).

Observaciones de implementación:

- `T0` y `T1` limpian `TF0/TF1` al entrar en ISR.
- `T2` reconoce interrupción por `TF2` o `EXF2`.
- `cpu_on_reti` restaura prioridad ISR previa.

## 6. Bajo consumo
### 6.1 IDLE
- `cpu_step` devuelve 0.
- El bucle de `cpu_run`/`cpu_run_timed` mantiene avance de periféricos con 1 ciclo virtual.
- Puede despertar por interrupción habilitada.

### 6.2 POWER DOWN
- No ejecuta instrucciones.
- No atiende interrupciones hasta que el sistema externo lo despierte con `cpu_wake`.

## 7. Integración recomendada
Checklist mínimo para una app host:

1. `cpu_init`.
2. Adjuntar mapa de memoria (`mem_map_attach`).
3. Adjuntar periféricos vía hooks SFR y/o hooks de tick.
4. Cargar firmware en CODE.
5. Ejecutar `cpu_run` o `cpu_run_timed`.

Ejemplo real de integración: `src/main.c`.

## 8. Ejemplo de código

```c
#include "cpu.h"
#include "timers.h"

static void timers_tick_hook(cpu_t *cpu, uint32_t cycles, void *user)
{
	(void)cpu;
	timers_tick((timers_t *)user, cycles);
}

void run_firmware(cpu_t *cpu, timers_t *timers)
{
	cpu_tick_entry_t hooks[] = {
		{ .fn = timers_tick_hook, .user = timers },
	};

	cpu_init(cpu);
	timers_init(timers, cpu);
	cpu_set_tick_hooks(cpu, hooks, MCS51_ARRAY_LEN(hooks));

	cpu_run(cpu, 100000);
}
```

## 9. Buenas prácticas

- Usar `cpu_step` para depuración fina por instrucción.
- Usar `cpu_run_timed` solo si necesitás pacing en tiempo real.
- Mantener periféricos en hooks de tick desacoplados del decoder.
- Evitar escribir `cpu->sfr[]` directo desde fuera del core; preferir APIs y hooks.
