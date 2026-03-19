# Implementación y uso de memoria CODE/XDATA

## 1. Modelo de memoria del emulador
El core separa los espacios de memoria típicos MCS-51:

- CODE (lectura de programa + opcional escritura).
- XDATA (memoria externa de datos).
- IRAM/SFR (internas al core, ya incluidas en `cpu_t`).

Este documento cubre CODE y XDATA, que se conectan mediante callbacks.

## 2. Capa de mapeo por regiones
`mem_map_t` define regiones para CODE y XDATA.

Cada región (`mem_map_region_t`) incluye:

- `base`: dirección inicial de 16 bits.
- `size`: tamaño en bytes.
- `read`: callback de lectura.
- `write`: callback de escritura.
- `user`: puntero de contexto de backend.

Implementación: `lib/mem_map.h` y `lib/mem_map.c`.

## 3. Resolución de direcciones
La búsqueda de región:

- Recorre linealmente la tabla de regiones.
- Verifica si la dirección cae en `[base, base + size)`.
- Usa la primera región válida encontrada.

Si no hay región o callback:

- Lectura retorna `0xFF`.
- Escritura se ignora.

## 4. Conexión al CPU
`mem_map_attach(cpu, mem)` instala internamente `cpu_mem_ops_t` para:

- `cpu_code_read/cpu_code_write`.
- `cpu_xdata_read/cpu_xdata_write`.

Desde ese momento, el fetch de opcodes y los accesos MOVX pasan por el mapa.

## 5. Patrón de uso recomendado
### 5.1 Caso simple (host)
- Reservar arrays de 64K para CODE y XDATA.
- Definir una región única para cada espacio.
- Usar callbacks tipo RAM plana.

Ese patrón está implementado en `src/main.c`.

### 5.2 Caso embebido
- CODE puede mapear a flash/ROM o buffer de firmware.
- XDATA puede mapear a SRAM externa, FRAM, o emulación en RAM.
- Se puede mezclar múltiples regiones (por ejemplo ROM + MMIO simulado).

## 6. Reglas importantes

- Mantener callbacks puros y rápidos (se llaman por instrucción).
- Evitar side-effects pesados dentro de `read` de CODE.
- Si necesitás MMIO en XDATA, documentar claramente qué rangos son especiales.

## 7. Diagnóstico
Síntomas típicos de wiring incorrecto:

- CPU se detiene con opcode inválido: probable CODE sin cargar o mapa mal configurado.
- Lecturas constantes en `0xFF`: región no encontrada o callback nulo.
- MOVX no modifica datos: write callback ausente o ruta XDATA no conectada.
