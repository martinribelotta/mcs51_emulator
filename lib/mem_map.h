#ifndef MCS51_MEM_MAP_H
#define MCS51_MEM_MAP_H

#include <stdint.h>
#include <stddef.h>

#include "cpu.h"

typedef uint8_t (*mem_map_read_hook)(const cpu_t *cpu, uint16_t addr, void *user);
typedef void (*mem_map_write_hook)(cpu_t *cpu, uint16_t addr, uint8_t value, void *user);

typedef struct {
    uint16_t base;
    uint32_t size;
    mem_map_read_hook read;
    mem_map_write_hook write;
    void *user;
} mem_map_region_t;

typedef struct {
    const mem_map_region_t *code_regions;
    size_t code_region_count;
    const mem_map_region_t *xdata_regions;
    size_t xdata_region_count;
} mem_map_t;

const mem_map_region_t *mem_map_find_code_region(const mem_map_t *mem, uint16_t addr);
const mem_map_region_t *mem_map_find_xdata_region(const mem_map_t *mem, uint16_t addr);
void mem_map_attach(cpu_t *cpu, const mem_map_t *mem);

#endif
