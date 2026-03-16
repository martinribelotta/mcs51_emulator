#include "mem_rom.h"

#include <string.h>

static uint8_t code_read_rom(const cpu_t *cpu, uint16_t addr, void *user)
{
    (void)cpu;
    mem_rom_t *mem = (mem_rom_t *)user;
    if (addr < mem->code_size) {
        return mem->code[addr];
    }
    return 0xFF;
}

static void code_write_ignored(cpu_t *cpu, uint16_t addr, uint8_t value, void *user)
{
    (void)cpu;
    (void)addr;
    (void)value;
    (void)user;
}

static uint8_t xdata_read_rom(const cpu_t *cpu, uint16_t addr, void *user)
{
    (void)cpu;
    mem_rom_t *mem = (mem_rom_t *)user;
    return mem->xdata[addr];
}

static void xdata_write_rom(cpu_t *cpu, uint16_t addr, uint8_t value, void *user)
{
    (void)cpu;
    mem_rom_t *mem = (mem_rom_t *)user;
    mem->xdata[addr] = value;
}

void mem_rom_init(mem_rom_t *mem, const uint8_t *code, size_t code_size)
{
    mem->code = code;
    mem->code_size = code_size;
    memset(mem->xdata, 0, sizeof(mem->xdata));
}

void mem_rom_set_code(mem_rom_t *mem, const uint8_t *code, size_t code_size)
{
    mem->code = code;
    mem->code_size = code_size;
}

void mem_rom_attach(cpu_t *cpu, mem_rom_t *mem)
{
    cpu_mem_ops_t ops;
    ops.code_read = code_read_rom;
    ops.code_write = code_write_ignored;
    ops.xdata_read = xdata_read_rom;
    ops.xdata_write = xdata_write_rom;
    cpu_set_mem_ops(cpu, &ops, mem);
}
