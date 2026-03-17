#ifndef MCS51_CPU_H
#define MCS51_CPU_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define MCS51_ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

#include "timing.h"

typedef struct cpu cpu_t;
typedef void (*cpu_trace_fn)(cpu_t *cpu, uint16_t pc, uint8_t opcode, const char *name, void *user);
typedef uint8_t (*cpu_code_read_fn)(const cpu_t *cpu, uint16_t addr, void *user);
typedef void (*cpu_code_write_fn)(cpu_t *cpu, uint16_t addr, uint8_t value, void *user);
typedef uint8_t (*cpu_xdata_read_fn)(const cpu_t *cpu, uint16_t addr, void *user);
typedef void (*cpu_xdata_write_fn)(cpu_t *cpu, uint16_t addr, uint8_t value, void *user);

typedef struct {
    cpu_code_read_fn code_read;
    cpu_code_write_fn code_write;
    cpu_xdata_read_fn xdata_read;
    cpu_xdata_write_fn xdata_write;
} cpu_mem_ops_t;

typedef void (*cpu_tick_fn)(cpu_t *cpu, uint32_t cycles, void *user);

typedef struct {
    cpu_tick_fn fn;
    void *user;
} cpu_tick_entry_t;


typedef uint64_t (*cpu_time_now_ns_fn)(void *user);
typedef void (*cpu_time_sleep_ns_fn)(uint64_t ns, void *user);

typedef struct {
    cpu_time_now_ns_fn now_ns;
    cpu_time_sleep_ns_fn sleep_ns;
    void *user;
} cpu_time_iface_t;

typedef uint8_t (*sfr_read_hook)(const cpu_t *cpu, uint8_t addr, void *user);
typedef void (*sfr_write_hook)(cpu_t *cpu, uint8_t addr, uint8_t value, void *user);

uint8_t cpu_sfr_read_acc(const cpu_t *cpu, uint8_t addr, void *user);
void cpu_sfr_write_acc(cpu_t *cpu, uint8_t addr, uint8_t value, void *user);
uint8_t cpu_sfr_read_b(const cpu_t *cpu, uint8_t addr, void *user);
void cpu_sfr_write_b(cpu_t *cpu, uint8_t addr, uint8_t value, void *user);
uint8_t cpu_sfr_read_psw(const cpu_t *cpu, uint8_t addr, void *user);
void cpu_sfr_write_psw(cpu_t *cpu, uint8_t addr, uint8_t value, void *user);
uint8_t cpu_sfr_read_sp(const cpu_t *cpu, uint8_t addr, void *user);
void cpu_sfr_write_sp(cpu_t *cpu, uint8_t addr, uint8_t value, void *user);
uint8_t cpu_sfr_read_dpl(const cpu_t *cpu, uint8_t addr, void *user);
void cpu_sfr_write_dpl(cpu_t *cpu, uint8_t addr, uint8_t value, void *user);
uint8_t cpu_sfr_read_dph(const cpu_t *cpu, uint8_t addr, void *user);
void cpu_sfr_write_dph(cpu_t *cpu, uint8_t addr, uint8_t value, void *user);

typedef struct {
    sfr_read_hook read;
    sfr_write_hook write;
    void *user;
} sfr_hook_t;

struct cpu {
    uint8_t acc;
    uint8_t b;
    uint8_t psw;
    uint8_t sp;
    uint16_t dptr;
    uint16_t pc;

    uint8_t iram[256];
    uint8_t sfr[128];
    sfr_hook_t sfr_hooks[128];
    void *sfr_user;

    bool halted;
    uint8_t last_opcode;
    const char *halt_reason;

    bool trace_enabled;
    cpu_trace_fn trace_fn;
    void *trace_user;

    cpu_mem_ops_t mem_ops;
    void *mem_user;

    const cpu_tick_entry_t *tick_hooks;
    uint8_t tick_hook_count;

    bool in_isr;
    uint8_t isr_prio;
    uint8_t isr_stack[2];
    uint8_t isr_depth;

    bool int0_level;
    bool int1_level;
    bool int0_prev_level;
    bool int1_prev_level;
};

// CPU_INIT_TEMPLATE_INIT: initializer for static CPU templates with default SFR hooks.
#define CPU_INIT_TEMPLATE_INIT { \
    .sp = 0x07, \
    .int0_level = true, \
    .int1_level = true, \
    .int0_prev_level = true, \
    .int1_prev_level = true, \
    .sfr_hooks = { \
        [0xE0 - 0x80] = { cpu_sfr_read_acc, cpu_sfr_write_acc, NULL }, \
        [0xF0 - 0x80] = { cpu_sfr_read_b, cpu_sfr_write_b, NULL }, \
        [0xD0 - 0x80] = { cpu_sfr_read_psw, cpu_sfr_write_psw, NULL }, \
        [0x81 - 0x80] = { cpu_sfr_read_sp, cpu_sfr_write_sp, NULL }, \
        [0x82 - 0x80] = { cpu_sfr_read_dpl, cpu_sfr_write_dpl, NULL }, \
        [0x83 - 0x80] = { cpu_sfr_read_dph, cpu_sfr_write_dph, NULL }, \
    }, \
}

// CPU_INIT_TEMPLATE: canonical const template instance.
extern const cpu_t CPU_INIT_TEMPLATE_CONST;
#define CPU_INIT_TEMPLATE CPU_INIT_TEMPLATE_CONST

// CPU_RESET_TEMPLATE: base values applied by cpu_reset (hooks/mem are preserved).
#define CPU_RESET_TEMPLATE ((cpu_t){ \
    .sp = 0x07, \
})

void cpu_init(cpu_t *cpu);
void cpu_reset(cpu_t *cpu);

uint8_t cpu_fetch8(cpu_t *cpu);
uint16_t cpu_fetch16(cpu_t *cpu);
uint8_t cpu_code_read(const cpu_t *cpu, uint16_t addr);
void cpu_code_write(cpu_t *cpu, uint16_t addr, uint8_t value);
uint8_t cpu_xdata_read(const cpu_t *cpu, uint16_t addr);
void cpu_xdata_write(cpu_t *cpu, uint16_t addr, uint8_t value);

uint8_t cpu_read_direct(cpu_t *cpu, uint8_t addr);
void cpu_write_direct(cpu_t *cpu, uint8_t addr, uint8_t value);

uint8_t cpu_read_bit(cpu_t *cpu, uint8_t bit_addr);
void cpu_write_bit(cpu_t *cpu, uint8_t bit_addr, bool value);

uint8_t cpu_step(cpu_t *cpu);
void cpu_run(cpu_t *cpu, uint64_t max_steps);
void cpu_run_timed(cpu_t *cpu,
                   uint64_t max_steps,
                   const timing_config_t *timing_cfg,
                   timing_state_t *timing_state,
                   const cpu_time_iface_t *time_iface);
void cpu_set_trace(cpu_t *cpu, bool enabled, cpu_trace_fn fn, void *user);
void cpu_set_sfr_hook(cpu_t *cpu, uint8_t addr, sfr_read_hook read, sfr_write_hook write, void *user);
void cpu_set_sfr_user(cpu_t *cpu, void *user);
void cpu_set_mem_ops(cpu_t *cpu, const cpu_mem_ops_t *ops, const void *user);
void cpu_set_tick_hooks(cpu_t *cpu, const cpu_tick_entry_t *hooks, uint8_t count);
void cpu_set_int0_level(cpu_t *cpu, bool level);
void cpu_set_int1_level(cpu_t *cpu, bool level);
void cpu_on_reti(cpu_t *cpu);

void cpu_set_carry(cpu_t *cpu, bool value);
bool cpu_get_carry(const cpu_t *cpu);
void cpu_set_aux_carry(cpu_t *cpu, bool value);
void cpu_set_overflow(cpu_t *cpu, bool value);

#endif
