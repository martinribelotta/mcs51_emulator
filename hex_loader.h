#ifndef MCS51_HEX_LOADER_H
#define MCS51_HEX_LOADER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "cpu.h"

bool hex_load_file(cpu_t *cpu, const char *path);
uint8_t *hex_load_file_malloc(const char *path, size_t *out_size);

#endif
