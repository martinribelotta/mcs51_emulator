#ifndef MCS51_CPU_H
#define MCS51_CPU_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct cpu cpu_t;
typedef void (*cpu_trace_fn)(cpu_t *cpu, uint16_t pc, uint8_t opcode, const char *name, void *user);

typedef uint8_t (*sfr_read_fn)(cpu_t *cpu, uint8_t addr);
typedef void (*sfr_write_fn)(cpu_t *cpu, uint8_t addr, uint8_t value);

struct cpu {
    uint8_t acc;
    uint8_t b;
    uint8_t psw;
    uint8_t sp;
    uint16_t dptr;
    uint16_t pc;

    uint8_t code[65536];
    uint8_t iram[256];
    uint8_t sfr[128];
    uint8_t xdata[65536];

    sfr_read_fn sfr_read;
    sfr_write_fn sfr_write;

    bool halted;
    uint8_t last_opcode;

    bool trace_enabled;
    cpu_trace_fn trace_fn;
    void *trace_user;
};

void cpu_init(cpu_t *cpu);
void cpu_reset(cpu_t *cpu);

uint8_t cpu_fetch8(cpu_t *cpu);
uint16_t cpu_fetch16(cpu_t *cpu);

uint8_t cpu_read_direct(cpu_t *cpu, uint8_t addr);
void cpu_write_direct(cpu_t *cpu, uint8_t addr, uint8_t value);

uint8_t cpu_read_bit(cpu_t *cpu, uint8_t bit_addr);
void cpu_write_bit(cpu_t *cpu, uint8_t bit_addr, bool value);

bool cpu_step(cpu_t *cpu);
void cpu_run(cpu_t *cpu, uint64_t max_steps);
void cpu_set_trace(cpu_t *cpu, bool enabled, cpu_trace_fn fn, void *user);

void cpu_set_carry(cpu_t *cpu, bool value);
bool cpu_get_carry(cpu_t *cpu);
void cpu_set_aux_carry(cpu_t *cpu, bool value);
void cpu_set_overflow(cpu_t *cpu, bool value);

#endif
