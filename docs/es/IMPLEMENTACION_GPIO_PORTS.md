# Implementación y uso de GPIO (puertos P0-P3)

## 1. Alcance
El módulo de puertos implementa el comportamiento de SFR `P0..P3` con callbacks hacia el entorno host.

Archivos: `lib/ports.h` y `lib/ports.c`.

## 2. Modelo de puertos
`ports_t` contiene:

- referencia al CPU,
- `latch[4]` (estado latcheado por puerto),
- callback de lectura externa (`read_cb`),
- callback de escritura (`write_cb`).

## 3. Semántica de lectura
En lectura de SFR de puerto:

1. Se toma latch interno.
2. Se combina con entrada externa (`read_cb`).
3. Se aplica máscara de pull-up modelada por puerto.

Implementación actual:

- `P0` sin pull-up interno (`0x00`).
- `P1/P2/P3` con pull-up (`0xFF`).

La salida final sigue la fórmula de puertos quasi-bidireccionales que está codificada en `port_read_effective`.

## 4. Semántica de escritura
En escritura de `P0..P3`:

- se actualiza latch,
- se refleja en SFR,
- se notifica al host por `write_cb` con:
  - nivel conducido,
  - máscara de bits realmente conducidos.

Esto permite separar bits en alta impedancia lógica vs bits conducidos.

## 5. Attach al CPU
`ports_init` instala hooks SFR para `P0`, `P1`, `P2`, `P3`.

También inicializa `latch[]` desde el valor SFR actual del CPU.

## 6. API de uso
### 6.1 Inicialización
1. Definir callbacks de lectura/escritura.
2. Llamar `ports_init`.
3. Si cambiás backend en runtime, usar `ports_set_callbacks`.

### 6.2 Integración típica
- `read_cb`: leer estado de pines externos (GPIO host, waveform, sensores, etc.).
- `write_cb`: aplicar estados a hardware o a un modelo externo.

## 7. Ejemplo en el proyecto
`src/main.c` muestra:

- `ports_read_stub` para inyectar señal RX serial por P3.
- `ports_write_stdout` para log de salidas de puerto.

## 8. Ejemplo de código

```c
#include <stdio.h>
#include "ports.h"

static uint8_t host_read_pin(uint8_t port, void *user)
{
  const uint8_t *ext_levels = (const uint8_t *)user;
  return ext_levels[port & 0x03u];
}

static void host_write_pin(uint8_t port, uint8_t level, uint8_t mask, void *user)
{
  (void)user;
  /* Enviar a logger, GPIO host o simulador externo */
  printf("P%u level=0x%02X mask=0x%02X\n", port, level, mask);
}

void attach_ports(cpu_t *cpu, ports_t *ports, uint8_t *ext_levels)
{
  ports_init(ports, cpu, host_read_pin, host_write_pin, ext_levels);
}
```

## 9. Recomendaciones

- Mantener `read_cb` libre de bloqueos.
- Evitar side-effects de escritura dentro de `read_cb`.
- Si necesitás flancos para timers en modo counter, derivarlos desde la misma capa GPIO y enviarlos al módulo `timers`.

## 10. Limitaciones actuales

- El modelo es suficiente para gran parte del firmware clásico, pero no emula fenómenos eléctricos finos (corriente, tiempos analógicos, contención de bus).
- Si necesitás precisión eléctrica, crear una capa adicional de I/O físico encima de este módulo.
