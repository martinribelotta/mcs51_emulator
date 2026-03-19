# Implementación y uso de hooks de registros y memoria

## 1. Qué problema resuelven
Los hooks permiten interceptar accesos del core a:

- SFR (registros especiales).
- CODE/XDATA (memoria externa).
- Ticks de ciclo (tiempo para periféricos).

Con esto, el core CPU queda desacoplado de periféricos y hardware host.

## 2. Hooks SFR
### 2.1 API
- `cpu_set_sfr_hook(cpu, addr, read, write, user)`

### 2.2 Semántica de usuario
Cada hook usa su puntero `user` propio (el configurado en `cpu_set_sfr_hook`).

### 2.3 Flujo real de acceso
- `cpu_read_direct` / `cpu_write_direct` detectan si la dirección está en SFR.
- Si existe hook, delegan en callback.
- Si no, operan directo sobre `cpu->sfr[]`.

## 3. Hooks de memoria externa
### 3.1 API
- `cpu_set_mem_ops(cpu, ops, user)`

`ops` define callbacks para CODE y XDATA.

### 3.2 Integración recomendada
No registrar a mano salvo necesidad especial; usar `mem_map_attach` para wiring por regiones.

## 4. Hooks de tick
### 4.1 API
- `cpu_set_tick_hooks(cpu, hooks, count)`

Cada entrada (`cpu_tick_entry_t`) recibe:

- puntero a CPU,
- ciclos de la instrucción ejecutada,
- contexto `user`.

### 4.2 Cuándo se ejecutan
En `cpu_run` y `cpu_run_timed`, justo después de `cpu_step` y antes de la siguiente instrucción.

Es el lugar correcto para:

- timers,
- UART,
- GPIO periódicos,
- co-simulación de periféricos.

## 5. Patrones de diseño

### 5.1 Periférico por módulo
Cada periférico expone:

1. función de attach (instala hooks SFR),
2. función de tick (avanza tiempo interno),
3. callbacks de integración con el host.

UART, Timers y Ports del proyecto siguen este patrón.

### 5.2 Evitar acoplamiento circular
- El hook debe tocar solo su SFR y su propio estado.
- La coordinación entre periféricos debe hacerse por hooks de tick o señales explícitas.

## 6. Errores comunes

- Instalar hook en dirección < 0x80: se ignora.
- Reusar `user` inválido o stack local que expira.
- Escribir SFR críticos sin respetar semántica de bits (por ejemplo flags de IRQ).

## 7. Checklist de integración

1. Definir estado del periférico.
2. Instalar hooks SFR necesarios.
3. Registrar tick hook si el periférico depende de ciclos.
4. Verificar interacción con interrupciones.
5. Agregar test funcional mínimo.
