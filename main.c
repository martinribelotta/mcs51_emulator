#include "cpu.h"
#include "mem_map.h"
#include "hex_loader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool hex_load_file(cpu_t *cpu, const char *path);

enum { RAM_8K_SIZE = 8 * 1024 };

static uint8_t ram8k_read(const cpu_t *cpu, uint16_t addr, void *user)
{
    (void)cpu;
    uint8_t *ram = (uint8_t *)user;
    return ram[addr];
}

static void ram8k_write(cpu_t *cpu, uint16_t addr, uint8_t value, void *user)
{
    (void)cpu;
    uint8_t *ram = (uint8_t *)user;
    ram[addr] = value;
}

static void trace_stdout(cpu_t *cpu, uint16_t pc, uint8_t opcode, const char *name, void *user)
{
    FILE *out = user ? (FILE *)user : stdout;
    fprintf(out,
            "%04X  %02X  %-16s  A=%02X PSW=%02X SP=%02X DPTR=%04X\n",
            pc,
            opcode,
            name,
            cpu->acc,
            cpu->psw,
            cpu->sp,
            cpu->dptr);
}

static void print_usage(const char *prog)
{
    printf("Usage: %s <firmware.hex> [max_steps] [--trace]\n", prog);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    cpu_t cpu;
    mem_map_t mem;
    uint8_t code8k[RAM_8K_SIZE];
    uint8_t xdata8k[RAM_8K_SIZE];
    mem_map_region_t code_regions[] = {
        {
            .base = 0x0000,
            .size = RAM_8K_SIZE,
            .read = ram8k_read,
            .write = ram8k_write,
            .user = code8k,
        },
    };
    mem_map_region_t xdata_regions[] = {
        {
            .base = 0x0000,
            .size = RAM_8K_SIZE,
            .read = ram8k_read,
            .write = ram8k_write,
            .user = xdata8k,
        },
    };
    cpu_init(&cpu);
    mem_map_init(&mem);
    mem_map_set_code_regions(&mem, code_regions, sizeof(code_regions) / sizeof(code_regions[0]));
    mem_map_set_xdata_regions(&mem, xdata_regions, sizeof(xdata_regions) / sizeof(xdata_regions[0]));
    mem_map_attach(&cpu, &mem);
    memset(code8k, 0xFF, sizeof(code8k));
    memset(xdata8k, 0x00, sizeof(xdata8k));

    if (!hex_load_file(&cpu, argv[1])) {
        fprintf(stderr, "Failed to load HEX file: %s\n", argv[1]);
        return 1;
    }

    uint64_t max_steps = 0;
    if (argc >= 3 && argv[2][0] != '-') {
        max_steps = (uint64_t)strtoull(argv[2], NULL, 10);
    }

    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "--trace") == 0) {
            cpu_set_trace(&cpu, true, trace_stdout, stdout);
        }
    }

    cpu_run(&cpu, max_steps);

    if (cpu.halted) {
        if (cpu.halt_reason) {
            printf("CPU halted at PC=0x%04X (opcode 0x%02X): %s\n",
                   cpu.pc, cpu.last_opcode, cpu.halt_reason);
        } else {
            printf("CPU halted at PC=0x%04X (opcode 0x%02X)\n",
                   cpu.pc, cpu.last_opcode);
        }
    } else {
        printf("CPU stopped after %llu steps at PC=0x%04X\n",
               (unsigned long long)max_steps, cpu.pc);
    }

    return 0;
}
