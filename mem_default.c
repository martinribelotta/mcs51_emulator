#include "mem_default.h"

#include <string.h>

static uint8_t code_read_default(const cpu_t *cpu, uint16_t addr, void *user)
{
    (void)cpu;
    mem_default_t *mem = (mem_default_t *)user;
    return mem->code[addr];
}

static void code_write_default(cpu_t *cpu, uint16_t addr, uint8_t value, void *user)
{
    (void)cpu;
    mem_default_t *mem = (mem_default_t *)user;
    mem->code[addr] = value;
}

static uint8_t xdata_read_default(const cpu_t *cpu, uint16_t addr, void *user)
{
    (void)cpu;
    mem_default_t *mem = (mem_default_t *)user;
    return mem->xdata[addr];
}

static void xdata_write_default(cpu_t *cpu, uint16_t addr, uint8_t value, void *user)
{
    (void)cpu;
    mem_default_t *mem = (mem_default_t *)user;
    mem->xdata[addr] = value;
}

void mem_default_init(mem_default_t *mem)
{
    memset(mem, 0, sizeof(*mem));
}

void mem_default_attach(cpu_t *cpu, mem_default_t *mem)
{
    cpu_mem_ops_t ops;
    ops.code_read = code_read_default;
    ops.code_write = code_write_default;
    ops.xdata_read = xdata_read_default;
    ops.xdata_write = xdata_write_default;
    cpu_set_mem_ops(cpu, &ops, mem);
}
