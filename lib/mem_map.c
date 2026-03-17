#include "mem_map.h"

#include <string.h>

static const mem_map_region_t *find_region(const mem_map_region_t *regions, size_t count, uint16_t addr)
{
    if (!regions || count == 0) {
        return NULL;
    }
    for (size_t i = 0; i < count; ++i) {
        const mem_map_region_t *region = &regions[i];
        if (region->size == 0) {
            continue;
        }
        uint32_t end = (uint32_t)region->base + (uint32_t)region->size;
        if ((uint32_t)addr >= region->base && (uint32_t)addr < end) {
            return region;
        }
    }
    return NULL;
}

static uint8_t code_read_map(const cpu_t *cpu, uint16_t addr, void *user)
{
    const mem_map_t *mem = (const mem_map_t *)user;
    const mem_map_region_t *region = mem_map_find_code_region(mem, addr);
    if (region && region->read) {
        return region->read(cpu, addr, region->user);
    }
    return 0xFF;
}

static void code_write_map(cpu_t *cpu, uint16_t addr, uint8_t value, void *user)
{
    const mem_map_t *mem = (const mem_map_t *)user;
    const mem_map_region_t *region = mem_map_find_code_region(mem, addr);
    if (region && region->write) {
        region->write(cpu, addr, value, region->user);
    }
}

static uint8_t xdata_read_map(const cpu_t *cpu, uint16_t addr, void *user)
{
    const mem_map_t *mem = (const mem_map_t *)user;
    const mem_map_region_t *region = mem_map_find_xdata_region(mem, addr);
    if (region && region->read) {
        return region->read(cpu, addr, region->user);
    }
    return 0xFF;
}

static void xdata_write_map(cpu_t *cpu, uint16_t addr, uint8_t value, void *user)
{
    const mem_map_t *mem = (const mem_map_t *)user;
    const mem_map_region_t *region = mem_map_find_xdata_region(mem, addr);
    if (region && region->write) {
        region->write(cpu, addr, value, region->user);
    }
}

const mem_map_region_t *mem_map_find_code_region(const mem_map_t *mem, uint16_t addr)
{
    if (!mem) {
        return NULL;
    }
    return find_region(mem->code_regions, mem->code_region_count, addr);
}

const mem_map_region_t *mem_map_find_xdata_region(const mem_map_t *mem, uint16_t addr)
{
    if (!mem) {
        return NULL;
    }
    return find_region(mem->xdata_regions, mem->xdata_region_count, addr);
}

void mem_map_attach(cpu_t *cpu, const mem_map_t *mem)
{
    cpu_mem_ops_t ops;
    ops.code_read = code_read_map;
    ops.code_write = code_write_map;
    ops.xdata_read = xdata_read_map;
    ops.xdata_write = xdata_write_map;
    cpu_set_mem_ops(cpu, &ops, mem);
}
