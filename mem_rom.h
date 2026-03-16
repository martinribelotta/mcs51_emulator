#ifndef MCS51_MEM_ROM_H
#define MCS51_MEM_ROM_H

#include <stdint.h>
#include <stddef.h>

#include "cpu.h"

typedef struct {
    const uint8_t *code;
    size_t code_size;
    uint8_t xdata[65536];
} mem_rom_t;

void mem_rom_init(mem_rom_t *mem, const uint8_t *code, size_t code_size);
void mem_rom_set_code(mem_rom_t *mem, const uint8_t *code, size_t code_size);
void mem_rom_attach(cpu_t *cpu, mem_rom_t *mem);

#endif
