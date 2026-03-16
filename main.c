#include "cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool hex_load_file(cpu_t *cpu, const char *path);

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
    cpu_init(&cpu);

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
        printf("CPU halted at PC=0x%04X (opcode 0x%02X)\n", cpu.pc, cpu.last_opcode);
    } else {
        printf("CPU stopped after %llu steps at PC=0x%04X\n",
               (unsigned long long)max_steps, cpu.pc);
    }

    return 0;
}
