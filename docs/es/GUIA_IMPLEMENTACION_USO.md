# Guía de implementación y uso del runtime

Este índice organiza la documentación técnica por subsistema.

## Documentos

1. CPU
   - `docs/es/IMPLEMENTACION_CPU.md`

2. Memoria CODE/XDATA
   - `docs/es/IMPLEMENTACION_MEMORIA_CODE_XDATA.md`

3. Hooks de registros y memoria
   - `docs/es/IMPLEMENTACION_HOOKS_REGISTROS_MEMORIA.md`

4. Serial UART
   - `docs/es/IMPLEMENTACION_SERIAL_UART.md`

5. Timers
   - `docs/es/IMPLEMENTACION_TIMERS.md`

6. GPIO / Ports
   - `docs/es/IMPLEMENTACION_GPIO_PORTS.md`

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

- El archivo `src/main.c` es una referencia funcional completa de wiring de subsistemas.
- `tests/test_runner.c` contiene pruebas unitarias útiles para validación de regresión.
