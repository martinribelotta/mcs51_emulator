#ifndef MCS51_MEM_DEFAULT_H
#define MCS51_MEM_DEFAULT_H

#include <stdint.h>
#include <stddef.h>

#include "cpu.h"

typedef struct {
    uint8_t code[65536];
    uint8_t xdata[65536];
} mem_default_t;

void mem_default_init(mem_default_t *mem);
void mem_default_attach(cpu_t *cpu, mem_default_t *mem);

#endif
