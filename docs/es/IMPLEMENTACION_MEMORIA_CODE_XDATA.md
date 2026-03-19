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

Ese patrón está implementado en el [código principal de integración](../../src/main.c).

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

## 8. Ejemplo de código

```c
#include "mem_map.h"

static uint8_t code_rom[65536];
static uint8_t xdata_ram[65536];

static uint8_t flat_read(const cpu_t *cpu, uint16_t addr, void *user)
{
	(void)cpu;
	uint8_t *mem = (uint8_t *)user;
	return mem[addr];
}

static void flat_write(cpu_t *cpu, uint16_t addr, uint8_t value, void *user)
{
	(void)cpu;
	uint8_t *mem = (uint8_t *)user;
	mem[addr] = value;
}

void attach_flat_memory(cpu_t *cpu)
{
	static const mem_map_region_t code_regions[] = {
		{ .base = 0x0000, .size = sizeof(code_rom), .read = flat_read, .write = NULL, .user = code_rom },
	};
	static const mem_map_region_t xdata_regions[] = {
		{ .base = 0x0000, .size = sizeof(xdata_ram), .read = flat_read, .write = flat_write, .user = xdata_ram },
	};
	static const mem_map_t map = {
		.code_regions = code_regions,
		.code_region_count = MCS51_ARRAY_LEN(code_regions),
		.xdata_regions = xdata_regions,
		.xdata_region_count = MCS51_ARRAY_LEN(xdata_regions),
	};

	mem_map_attach(cpu, &map);
}
```

## 9. Enlaces relacionados

- Guía general: [Guía de implementación y uso](GUIA_IMPLEMENTACION_USO.md)
- CPU: [Implementación de CPU](IMPLEMENTACION_CPU.md)
- Hooks de memoria: [Implementación de hooks de registros y memoria](IMPLEMENTACION_HOOKS_REGISTROS_MEMORIA.md)
- Versión en inglés: [Documentación en inglés de memoria CODE/XDATA](../en/CODE_XDATA_MEMORY_IMPLEMENTATION.md)
