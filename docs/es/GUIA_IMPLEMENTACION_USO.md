# Guía de implementación y uso del runtime

Este índice organiza la documentación técnica por subsistema.

## Documentos

1. CPU
   - [Implementación de CPU](IMPLEMENTACION_CPU.md)

2. Memoria CODE/XDATA
   - [Implementación de memoria CODE/XDATA](IMPLEMENTACION_MEMORIA_CODE_XDATA.md)

3. Hooks de registros y memoria
   - [Implementación de hooks de registros y memoria](IMPLEMENTACION_HOOKS_REGISTROS_MEMORIA.md)

4. Serial UART
   - [Implementación de UART serial](IMPLEMENTACION_SERIAL_UART.md)

5. Timers
   - [Implementación de timers](IMPLEMENTACION_TIMERS.md)

6. GPIO / Ports
   - [Implementación de GPIO / puertos](IMPLEMENTACION_GPIO_PORTS.md)

## Orden de lectura recomendado

### A. Para entender arquitectura
1) CPU
2) Memoria CODE/XDATA
3) Hooks

### B. Para integrar periféricos
4) UART
5) Timers
6) GPIO

### C. Para portar a otra MCU
- Leer Hooks + Timers + GPIO en conjunto.
- Definir una capa HAL que traduzca eventos físicos del host a señales lógicas del emulador.

## Ruta de integración mínima

1. Inicializar CPU.
2. Adjuntar mapa de memoria CODE/XDATA.
3. Adjuntar UART, Timers y GPIO por hooks SFR.
4. Registrar hooks de tick de periféricos.
5. Cargar firmware.
6. Ejecutar con `cpu_run` o `cpu_run_timed`.

## Notas

- El [archivo principal de integración](../../src/main.c) es una referencia funcional completa de wiring de subsistemas.
- La [suite de pruebas de regresión](../../tests/test_runner.c) contiene pruebas unitarias útiles para validación de regresión.

## Enlaces cruzados

- Guía en inglés: [Guía de implementación en inglés](../en/IMPLEMENTATION_USAGE_GUIDE.md)
- CPU ↔ Memoria: [Implementación de CPU](IMPLEMENTACION_CPU.md) y [Implementación de memoria CODE/XDATA](IMPLEMENTACION_MEMORIA_CODE_XDATA.md)
- Hooks ↔ Periféricos: [Implementación de hooks de registros y memoria](IMPLEMENTACION_HOOKS_REGISTROS_MEMORIA.md), [Implementación de UART serial](IMPLEMENTACION_SERIAL_UART.md), [Implementación de timers](IMPLEMENTACION_TIMERS.md), [Implementación de GPIO / puertos](IMPLEMENTACION_GPIO_PORTS.md)
