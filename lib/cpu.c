#include "cpu.h"
#include "decoder.h"
#include "instr_impl.h"
#include "instr_list.h"

#include <string.h>

#define PSW_CY 0x80
#define PSW_AC 0x40
#define PSW_OV 0x04
#define PSW_P  0x01

#define SFR_SP   0x81
#define SFR_DPL  0x82
#define SFR_DPH  0x83
#define SFR_TCON 0x88
#define SFR_TMOD 0x89
#define SFR_TL0  0x8A
#define SFR_TL1  0x8B
#define SFR_TH0  0x8C
#define SFR_TH1  0x8D
#define SFR_PSW  0xD0
#define SFR_PCON 0x87
#define SFR_IE   0xA8
#define SFR_IP   0xB8
#define SFR_ACC  0xE0
#define SFR_B    0xF0
#define SFR_P0   0x80
#define SFR_P1   0x90
#define SFR_P2   0xA0
#define SFR_P3   0xB0
#define SFR_SCON 0x98
#define SFR_SBUF 0x99
#define SFR_T2CON 0xC8
#define SFR_RCAP2L 0xCA
#define SFR_RCAP2H 0xCB
#define SFR_TL2  0xCC
#define SFR_TH2  0xCD

#define IE_EA  0x80
#define IE_EX0 0x01
#define IE_ET0 0x02
#define IE_EX1 0x04
#define IE_ET1 0x08
#define IE_ES  0x10
#define IE_ET2 0x20

#define IP_PX0 0x01
#define IP_PT0 0x02
#define IP_PX1 0x04
#define IP_PT1 0x08
#define IP_PS  0x10
#define IP_PT2 0x20

#define TCON_IT0 0x01
#define TCON_IE0 0x02
#define TCON_IT1 0x04
#define TCON_IE1 0x08
#define TCON_TF0 0x20
#define TCON_TF1 0x80

#define PCON_IDL 0x01
#define PCON_PD  0x02

#define T2CON_TF2  0x80
#define T2CON_EXF2 0x40

#define SCON_RI  0x01
#define SCON_TI  0x02

#if defined(__GNUC__) || defined(__clang__)
#define MCS51_LIKELY(x)   __builtin_expect(!!(x), 1)
#define MCS51_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define MCS51_WEAK __attribute__((weak))
#else
#define MCS51_LIKELY(x)   (x)
#define MCS51_UNLIKELY(x) (x)
#define MCS51_WEAK
#endif

static void cpu_check_interrupts(cpu_t *cpu);
static void cpu_apply_pcon(cpu_t *cpu, uint8_t value);

static cpu_run_timed_profiler_t g_run_timed_profiler = {
    .read_cycles = NULL,
    .on_sample = NULL,
    .user = NULL,
};

static uint8_t sfr_get(const cpu_t *cpu, uint8_t addr)
{
    return cpu->sfr[(uint8_t)(addr - 0x80)];
}

static void sfr_set(cpu_t *cpu, uint8_t addr, uint8_t value)
{
    cpu->sfr[(uint8_t)(addr - 0x80)] = value;
}

uint8_t cpu_sfr_read_acc(const cpu_t *cpu, uint8_t addr, void *user)
{
    (void)addr;
    (void)user;
    return cpu->acc;
}

void cpu_sfr_write_acc(cpu_t *cpu, uint8_t addr, uint8_t value, void *user)
{
    (void)addr;
    (void)user;
    cpu->acc = value;
}

uint8_t cpu_sfr_read_b(const cpu_t *cpu, uint8_t addr, void *user)
{
    (void)addr;
    (void)user;
    return cpu->b;
}

void cpu_sfr_write_b(cpu_t *cpu, uint8_t addr, uint8_t value, void *user)
{
    (void)addr;
    (void)user;
    cpu->b = value;
}

uint8_t cpu_sfr_read_psw(const cpu_t *cpu, uint8_t addr, void *user)
{
    (void)addr;
    (void)user;
    return cpu->psw;
}

void cpu_sfr_write_psw(cpu_t *cpu, uint8_t addr, uint8_t value, void *user)
{
    (void)addr;
    (void)user;
    cpu->psw = value;
}

uint8_t cpu_sfr_read_sp(const cpu_t *cpu, uint8_t addr, void *user)
{
    (void)addr;
    (void)user;
    return cpu->sp;
}

void cpu_sfr_write_sp(cpu_t *cpu, uint8_t addr, uint8_t value, void *user)
{
    (void)addr;
    (void)user;
    cpu->sp = value;
}

uint8_t cpu_sfr_read_dpl(const cpu_t *cpu, uint8_t addr, void *user)
{
    (void)addr;
    (void)user;
    return (uint8_t)(cpu->dptr & 0xFF);
}

uint8_t cpu_sfr_read_dph(const cpu_t *cpu, uint8_t addr, void *user)
{
    (void)addr;
    (void)user;
    return (uint8_t)(cpu->dptr >> 8);
}

void cpu_sfr_write_dpl(cpu_t *cpu, uint8_t addr, uint8_t value, void *user)
{
    (void)addr;
    (void)user;
    cpu->dptr = (uint16_t)((cpu->dptr & 0xFF00) | value);
}

void cpu_sfr_write_dph(cpu_t *cpu, uint8_t addr, uint8_t value, void *user)
{
    (void)addr;
    (void)user;
    cpu->dptr = (uint16_t)((cpu->dptr & 0x00FF) | ((uint16_t)value << 8));
}

const cpu_t CPU_INIT_TEMPLATE_CONST = CPU_INIT_TEMPLATE_INIT;

void cpu_init(cpu_t *cpu)
{
    if (!cpu) {
        return;
    }
    *cpu = CPU_INIT_TEMPLATE;
}

void cpu_reset(cpu_t *cpu)
{
    if (!cpu) {
        return;
    }
    cpu_t reset = CPU_RESET_TEMPLATE;
    memcpy(reset.iram, cpu->iram, sizeof(cpu->iram));
    memcpy(reset.sfr_hooks, cpu->sfr_hooks, sizeof(cpu->sfr_hooks));
    reset.mem_ops = cpu->mem_ops;
    reset.mem_user = cpu->mem_user;
    reset.tick_hooks = cpu->tick_hooks;
    reset.tick_hook_count = cpu->tick_hook_count;
    reset.int0_level = cpu->int0_level;
    reset.int1_level = cpu->int1_level;
    reset.int0_prev_level = cpu->int0_prev_level;
    reset.int1_prev_level = cpu->int1_prev_level;
    *cpu = reset;

    cpu->acc = 0x00;
    cpu->b = 0x00;
    cpu->psw = 0x00;
    cpu->sp = 0x07;
    cpu->dptr = 0x0000;
    cpu->pc = 0x0000;

    cpu->halted = false;
    cpu->last_opcode = 0x00;
    cpu->halt_reason = NULL;
    cpu->trace_enabled = false;
    cpu->trace_fn = NULL;
    cpu->trace_user = NULL;
    cpu->idle = false;
    cpu->power_down = false;

    memset(cpu->sfr, 0x00, sizeof(cpu->sfr));
    sfr_set(cpu, SFR_ACC, 0x00);
    sfr_set(cpu, SFR_B, 0x00);
    sfr_set(cpu, SFR_PSW, 0x00);
    sfr_set(cpu, SFR_SP, 0x07);
    sfr_set(cpu, SFR_DPL, 0x00);
    sfr_set(cpu, SFR_DPH, 0x00);
    sfr_set(cpu, SFR_PCON, 0x00);
    sfr_set(cpu, SFR_TCON, 0x00);
    sfr_set(cpu, SFR_TMOD, 0x00);
    sfr_set(cpu, SFR_TL0, 0x00);
    sfr_set(cpu, SFR_TL1, 0x00);
    sfr_set(cpu, SFR_TH0, 0x00);
    sfr_set(cpu, SFR_TH1, 0x00);
    sfr_set(cpu, SFR_T2CON, 0x00);
    sfr_set(cpu, SFR_RCAP2L, 0x00);
    sfr_set(cpu, SFR_RCAP2H, 0x00);
    sfr_set(cpu, SFR_TL2, 0x00);
    sfr_set(cpu, SFR_TH2, 0x00);
    sfr_set(cpu, SFR_SCON, 0x00);
    sfr_set(cpu, SFR_SBUF, 0x00);
    sfr_set(cpu, SFR_IE, 0x00);
    sfr_set(cpu, SFR_IP, 0x00);
    sfr_set(cpu, SFR_P0, 0xFF);
    sfr_set(cpu, SFR_P1, 0xFF);
    sfr_set(cpu, SFR_P2, 0xFF);
    sfr_set(cpu, SFR_P3, 0xFF);
}

uint8_t cpu_fetch8(cpu_t *cpu)
{
    uint8_t value = cpu_code_read(cpu, cpu->pc);
    cpu->pc++;
    return value;
}

uint16_t cpu_fetch16(cpu_t *cpu)
{
    uint8_t hi = cpu_fetch8(cpu);
    uint8_t lo = cpu_fetch8(cpu);
    return (uint16_t)((hi << 8) | lo);
}

uint8_t cpu_read_direct(cpu_t *cpu, uint8_t addr)
{
    if (MCS51_LIKELY(addr < 0x80)) {
        return cpu->iram[addr];
    }
    uint8_t idx = (uint8_t)(addr - 0x80);
    if (MCS51_UNLIKELY(cpu->sfr_hooks[idx].read)) {
        void *user = cpu->sfr_hooks[idx].user;
        return cpu->sfr_hooks[idx].read(cpu, addr, user);
    }
    return cpu->sfr[idx];
}

void cpu_write_direct(cpu_t *cpu, uint8_t addr, uint8_t value)
{
    if (MCS51_LIKELY(addr < 0x80)) {
        cpu->iram[addr] = value;
        return;
    }
    uint8_t idx = (uint8_t)(addr - 0x80);
    if (MCS51_UNLIKELY(cpu->sfr_hooks[idx].write)) {
        void *user = cpu->sfr_hooks[idx].user;
        cpu->sfr_hooks[idx].write(cpu, addr, value, user);
        if (addr == SFR_PCON) {
            cpu_apply_pcon(cpu, sfr_get(cpu, SFR_PCON));
        }
        return;
    }
    cpu->sfr[idx] = value;
    if (addr == SFR_PCON) {
        cpu_apply_pcon(cpu, value);
    }
}

MCS51_WEAK uint8_t emulator_code_read(const cpu_t *cpu_ctx, uint16_t code_addr)
{
    if (cpu_ctx->mem_ops.code_read) {
        return cpu_ctx->mem_ops.code_read(cpu_ctx, code_addr, cpu_ctx->mem_user);
    }
    return 0xFF;
}

uint8_t cpu_code_read(const cpu_t *cpu, uint16_t addr)
{
    return emulator_code_read(cpu, addr);
}

MCS51_WEAK void emulator_code_write(cpu_t *cpu_ctx, uint16_t code_addr, uint8_t value)
{
    if (cpu_ctx->mem_ops.code_write) {
        cpu_ctx->mem_ops.code_write(cpu_ctx, code_addr, value, cpu_ctx->mem_user);
    }
}

void cpu_code_write(cpu_t *cpu, uint16_t addr, uint8_t value)
{
    emulator_code_write(cpu, addr, value);
}

MCS51_WEAK uint8_t emulator_xdata_read(const cpu_t *cpu_ctx, uint16_t xdata_addr)
{
    if (cpu_ctx->mem_ops.xdata_read) {
        return cpu_ctx->mem_ops.xdata_read(cpu_ctx, xdata_addr, cpu_ctx->mem_user);
    }
    return 0xFF;
}

uint8_t cpu_xdata_read(const cpu_t *cpu, uint16_t addr)
{
    return emulator_xdata_read(cpu, addr);
}

MCS51_WEAK void emulator_xdata_write(cpu_t *cpu_ctx, uint16_t xdata_addr, uint8_t value)
{
    if (cpu_ctx->mem_ops.xdata_write) {
        cpu_ctx->mem_ops.xdata_write(cpu_ctx, xdata_addr, value, cpu_ctx->mem_user);
    }
}

void cpu_xdata_write(cpu_t *cpu, uint16_t addr, uint8_t value)
{
    emulator_xdata_write(cpu, addr, value);
}

static inline void bit_addr_to_byte_bit(uint8_t bit_addr, uint8_t *byte_addr, uint8_t *bit_pos)
{
    if (bit_addr < 0x80) {
        uint8_t base = (uint8_t)(0x20 + ((bit_addr >> 3) & 0x0F));
        *byte_addr = base;
        *bit_pos = (uint8_t)(bit_addr & 0x07);
        return;
    }
    *byte_addr = (uint8_t)(bit_addr & 0xF8);
    *bit_pos = (uint8_t)(bit_addr & 0x07);
}

uint8_t cpu_read_bit(cpu_t *cpu, uint8_t bit_addr)
{
    uint8_t byte_addr = 0;
    uint8_t bit_pos = 0;
    bit_addr_to_byte_bit(bit_addr, &byte_addr, &bit_pos);
    return (uint8_t)((cpu_read_direct(cpu, byte_addr) >> bit_pos) & 0x01);
}

void cpu_write_bit(cpu_t *cpu, uint8_t bit_addr, bool value)
{
    uint8_t byte_addr = 0;
    uint8_t bit_pos = 0;
    bit_addr_to_byte_bit(bit_addr, &byte_addr, &bit_pos);

    uint8_t current = cpu_read_direct(cpu, byte_addr);
    if (value) {
        current |= (uint8_t)(1u << bit_pos);
    } else {
        current &= (uint8_t)~(1u << bit_pos);
    }
    cpu_write_direct(cpu, byte_addr, current);
}

static const uint8_t k_opcode_cycles[256] = {
#define X(op, mn, dst, src, size, cycles, name) [op] = cycles,
    INSTR_LIST(X)
#undef X
};

static inline uint8_t cpu_reg_bank_base_fast(const cpu_t *cpu)
{
    return (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
}

static inline void cpu_add_a_fast(cpu_t *cpu, uint8_t value)
{
    uint8_t acc = cpu->acc;
    uint16_t sum = (uint16_t)acc + (uint16_t)value;
    uint8_t result = (uint8_t)sum;
    cpu->acc = result;

    uint8_t psw = cpu->psw;
    if (sum > 0xFFu) {
        psw |= PSW_CY;
    } else {
        psw &= (uint8_t)~PSW_CY;
    }
    if (((acc & 0x0Fu) + (value & 0x0Fu)) > 0x0Fu) {
        psw |= PSW_AC;
    } else {
        psw &= (uint8_t)~PSW_AC;
    }
    if (((~(acc ^ value)) & (acc ^ result) & 0x80u) != 0u) {
        psw |= PSW_OV;
    } else {
        psw &= (uint8_t)~PSW_OV;
    }
    cpu->psw = psw;
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((optimize("O3")))
#endif
uint8_t cpu_step(cpu_t *cpu)
{
    if (MCS51_UNLIKELY(cpu->power_down || cpu->idle)) {
        return 0;
    }
    if (MCS51_UNLIKELY(cpu->halted)) {
        return 0;
    }

    uint16_t pc_before = cpu->pc;
    uint8_t opcode = cpu_fetch8(cpu);
    cpu->last_opcode = opcode;
    uint8_t acc_before = cpu->acc;
    uint8_t cycles = k_opcode_cycles[opcode];
    uint8_t bank = 0;
    uint8_t addr = 0;
    uint8_t value = 0;
    uint8_t rhs = 0;
    uint8_t bit = 0;
    uint8_t idx = 0;
    uint8_t imm = 0;
    int8_t rel = 0;
    uint16_t addr16 = 0;
    uint16_t ret = 0;
    uint8_t psw = 0;

    switch (opcode) {
    case 0x00: /* MN_NOP */
        break;

    case 0x01: /* MN_AJMP */ case 0x21: /* MN_AJMP */ case 0x41: /* MN_AJMP */ case 0x61: /* MN_AJMP */
    case 0x81: /* MN_AJMP */ case 0xA1: /* MN_AJMP */ case 0xC1: /* MN_AJMP */ case 0xE1: /* MN_AJMP */
        imm = cpu_fetch8(cpu);
        addr16 = (uint16_t)(((opcode & 0xE0u) << 3) | imm);
        cpu->pc = (uint16_t)((cpu->pc & 0xF800u) | (addr16 & 0x07FFu));
        break;

    case 0x02: /* MN_LJMP */
        addr16 = cpu_fetch16(cpu);
        cpu->pc = addr16;
        break;

    case 0x03: /* MN_RR */
        cpu->acc = (uint8_t)((cpu->acc >> 1) | (cpu->acc << 7));
        break;

    case 0x04: /* MN_INC */
        cpu->acc++;
        break;
    case 0x05: /* MN_INC */
        addr = cpu_fetch8(cpu);
        value = cpu_read_direct(cpu, addr);
        cpu_write_direct(cpu, addr, (uint8_t)(value + 1u));
        break;
    case 0x06: /* MN_INC */ case 0x07: /* MN_INC */
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        idx = (uint8_t)(bank + (opcode & 0x01u));
        addr = cpu->iram[idx];
        cpu->iram[addr]++;
        break;
    case 0x08: /* MN_INC */ case 0x09: /* MN_INC */ case 0x0A: /* MN_INC */ case 0x0B: /* MN_INC */
    case 0x0C: /* MN_INC */ case 0x0D: /* MN_INC */ case 0x0E: /* MN_INC */ case 0x0F: /* MN_INC */
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        idx = (uint8_t)(bank + (opcode & 0x07u));
        cpu->iram[idx]++;
        break;

    case 0x10: /* MN_JBC */
        bit = cpu_fetch8(cpu);
        rel = (int8_t)cpu_fetch8(cpu);
        if (cpu_read_bit(cpu, bit) != 0) {
            cpu_write_bit(cpu, bit, false);
            cpu->pc = (uint16_t)(cpu->pc + rel);
        }
        break;

    case 0x11: /* MN_ACALL */ case 0x31: /* MN_ACALL */ case 0x51: /* MN_ACALL */ case 0x71: /* MN_ACALL */
    case 0x91: /* MN_ACALL */ case 0xB1: /* MN_ACALL */ case 0xD1: /* MN_ACALL */ case 0xF1: /* MN_ACALL */
        imm = cpu_fetch8(cpu);
        addr16 = (uint16_t)(((opcode & 0xE0u) << 3) | imm);
        ret = cpu->pc;
        cpu->sp++;
        cpu->iram[cpu->sp] = (uint8_t)(ret & 0xFFu);
        cpu->sp++;
        cpu->iram[cpu->sp] = (uint8_t)(ret >> 8);
        cpu->pc = (uint16_t)((cpu->pc & 0xF800u) | (addr16 & 0x07FFu));
        break;

    case 0x12: /* MN_LCALL */
        addr16 = cpu_fetch16(cpu);
        ret = cpu->pc;
        cpu->sp++;
        cpu->iram[cpu->sp] = (uint8_t)(ret & 0xFFu);
        cpu->sp++;
        cpu->iram[cpu->sp] = (uint8_t)(ret >> 8);
        cpu->pc = addr16;
        break;

    case 0x13: /* MN_RRC */
        value = cpu->acc;
        cpu->acc = (uint8_t)((value >> 1) | ((cpu->psw & PSW_CY) ? 0x80u : 0u));
        if (value & 0x01u) {
            cpu->psw |= PSW_CY;
        } else {
            cpu->psw &= (uint8_t)~PSW_CY;
        }
        break;

    case 0x14: /* MN_DEC */
        cpu->acc--;
        break;
    case 0x15: /* MN_DEC */
        addr = cpu_fetch8(cpu);
        value = cpu_read_direct(cpu, addr);
        cpu_write_direct(cpu, addr, (uint8_t)(value - 1u));
        break;
    case 0x16: /* MN_DEC */ case 0x17: /* MN_DEC */
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        idx = (uint8_t)(bank + (opcode & 0x01u));
        addr = cpu->iram[idx];
        cpu->iram[addr]--;
        break;
    case 0x18: /* MN_DEC */ case 0x19: /* MN_DEC */ case 0x1A: /* MN_DEC */ case 0x1B: /* MN_DEC */
    case 0x1C: /* MN_DEC */ case 0x1D: /* MN_DEC */ case 0x1E: /* MN_DEC */ case 0x1F: /* MN_DEC */
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        idx = (uint8_t)(bank + (opcode & 0x07u));
        cpu->iram[idx]--;
        break;

    case 0x20: /* MN_JB */
        bit = cpu_fetch8(cpu);
        rel = (int8_t)cpu_fetch8(cpu);
        if (cpu_read_bit(cpu, bit) != 0) {
            cpu->pc = (uint16_t)(cpu->pc + rel);
        }
        break;

    case 0x22: /* MN_RET */
        value = cpu->iram[cpu->sp--];
        rhs = cpu->iram[cpu->sp--];
        cpu->pc = (uint16_t)(((uint16_t)value << 8) | rhs);
        break;

    case 0x23: /* MN_RL */
        cpu->acc = (uint8_t)((cpu->acc << 1) | (cpu->acc >> 7));
        break;

    case 0x24: /* MN_ADD */
        imm = cpu_fetch8(cpu);
        value = cpu->acc;
        addr16 = (uint16_t)value + (uint16_t)imm;
        cpu->acc = (uint8_t)addr16;
        psw = cpu->psw;
        if (addr16 > 0xFFu) psw |= PSW_CY; else psw &= (uint8_t)~PSW_CY;
        if (((value & 0x0Fu) + (imm & 0x0Fu)) > 0x0Fu) psw |= PSW_AC; else psw &= (uint8_t)~PSW_AC;
        if (((~(value ^ imm)) & (value ^ cpu->acc) & 0x80u) != 0u) psw |= PSW_OV; else psw &= (uint8_t)~PSW_OV;
        cpu->psw = psw;
        break;
    case 0x25: /* MN_ADD */
        addr = cpu_fetch8(cpu);
        imm = cpu_read_direct(cpu, addr);
        value = cpu->acc;
        addr16 = (uint16_t)value + (uint16_t)imm;
        cpu->acc = (uint8_t)addr16;
        psw = cpu->psw;
        if (addr16 > 0xFFu) psw |= PSW_CY; else psw &= (uint8_t)~PSW_CY;
        if (((value & 0x0Fu) + (imm & 0x0Fu)) > 0x0Fu) psw |= PSW_AC; else psw &= (uint8_t)~PSW_AC;
        if (((~(value ^ imm)) & (value ^ cpu->acc) & 0x80u) != 0u) psw |= PSW_OV; else psw &= (uint8_t)~PSW_OV;
        cpu->psw = psw;
        break;
    case 0x26: /* MN_ADD */ case 0x27: /* MN_ADD */
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        addr = cpu->iram[(uint8_t)(bank + (opcode & 0x01u))];
        imm = cpu->iram[addr];
        value = cpu->acc;
        addr16 = (uint16_t)value + (uint16_t)imm;
        cpu->acc = (uint8_t)addr16;
        psw = cpu->psw;
        if (addr16 > 0xFFu) psw |= PSW_CY; else psw &= (uint8_t)~PSW_CY;
        if (((value & 0x0Fu) + (imm & 0x0Fu)) > 0x0Fu) psw |= PSW_AC; else psw &= (uint8_t)~PSW_AC;
        if (((~(value ^ imm)) & (value ^ cpu->acc) & 0x80u) != 0u) psw |= PSW_OV; else psw &= (uint8_t)~PSW_OV;
        cpu->psw = psw;
        break;
    case 0x28: /* MN_ADD */ case 0x29: /* MN_ADD */ case 0x2A: /* MN_ADD */ case 0x2B: /* MN_ADD */
    case 0x2C: /* MN_ADD */ case 0x2D: /* MN_ADD */ case 0x2E: /* MN_ADD */ case 0x2F: /* MN_ADD */
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        imm = cpu->iram[(uint8_t)(bank + (opcode & 0x07u))];
        value = cpu->acc;
        addr16 = (uint16_t)value + (uint16_t)imm;
        cpu->acc = (uint8_t)addr16;
        psw = cpu->psw;
        if (addr16 > 0xFFu) psw |= PSW_CY; else psw &= (uint8_t)~PSW_CY;
        if (((value & 0x0Fu) + (imm & 0x0Fu)) > 0x0Fu) psw |= PSW_AC; else psw &= (uint8_t)~PSW_AC;
        if (((~(value ^ imm)) & (value ^ cpu->acc) & 0x80u) != 0u) psw |= PSW_OV; else psw &= (uint8_t)~PSW_OV;
        cpu->psw = psw;
        break;

    case 0x30: /* MN_JNB */
        bit = cpu_fetch8(cpu);
        rel = (int8_t)cpu_fetch8(cpu);
        if (cpu_read_bit(cpu, bit) == 0) {
            cpu->pc = (uint16_t)(cpu->pc + rel);
        }
        break;

    case 0x32: /* MN_RETI */
        value = cpu->iram[cpu->sp--];
        rhs = cpu->iram[cpu->sp--];
        cpu->pc = (uint16_t)(((uint16_t)value << 8) | rhs);
        cpu_on_reti(cpu);
        break;

    case 0x33: /* MN_RLC */
        value = cpu->acc;
        cpu->acc = (uint8_t)((value << 1) | ((cpu->psw & PSW_CY) ? 1u : 0u));
        if (value & 0x80u) {
            cpu->psw |= PSW_CY;
        } else {
            cpu->psw &= (uint8_t)~PSW_CY;
        }
        break;

    case 0x34: /* MN_ADDC */
        imm = cpu_fetch8(cpu);
        value = cpu->acc;
        rhs = (uint8_t)((cpu->psw & PSW_CY) ? 1u : 0u);
        addr16 = (uint16_t)value + (uint16_t)imm + (uint16_t)rhs;
        cpu->acc = (uint8_t)addr16;
        psw = cpu->psw;
        if (addr16 > 0xFFu) psw |= PSW_CY; else psw &= (uint8_t)~PSW_CY;
        if (((value & 0x0Fu) + (imm & 0x0Fu) + rhs) > 0x0Fu) psw |= PSW_AC; else psw &= (uint8_t)~PSW_AC;
        if (((~(value ^ (uint8_t)(imm + rhs))) & (value ^ cpu->acc) & 0x80u) != 0u) psw |= PSW_OV; else psw &= (uint8_t)~PSW_OV;
        cpu->psw = psw;
        break;
    case 0x35: /* MN_ADDC */
        addr = cpu_fetch8(cpu);
        imm = cpu_read_direct(cpu, addr);
        value = cpu->acc;
        rhs = (uint8_t)((cpu->psw & PSW_CY) ? 1u : 0u);
        addr16 = (uint16_t)value + (uint16_t)imm + (uint16_t)rhs;
        cpu->acc = (uint8_t)addr16;
        psw = cpu->psw;
        if (addr16 > 0xFFu) psw |= PSW_CY; else psw &= (uint8_t)~PSW_CY;
        if (((value & 0x0Fu) + (imm & 0x0Fu) + rhs) > 0x0Fu) psw |= PSW_AC; else psw &= (uint8_t)~PSW_AC;
        if (((~(value ^ (uint8_t)(imm + rhs))) & (value ^ cpu->acc) & 0x80u) != 0u) psw |= PSW_OV; else psw &= (uint8_t)~PSW_OV;
        cpu->psw = psw;
        break;
    case 0x36: /* MN_ADDC */ case 0x37: /* MN_ADDC */
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        addr = cpu->iram[(uint8_t)(bank + (opcode & 0x01u))];
        imm = cpu->iram[addr];
        value = cpu->acc;
        rhs = (uint8_t)((cpu->psw & PSW_CY) ? 1u : 0u);
        addr16 = (uint16_t)value + (uint16_t)imm + (uint16_t)rhs;
        cpu->acc = (uint8_t)addr16;
        psw = cpu->psw;
        if (addr16 > 0xFFu) psw |= PSW_CY; else psw &= (uint8_t)~PSW_CY;
        if (((value & 0x0Fu) + (imm & 0x0Fu) + rhs) > 0x0Fu) psw |= PSW_AC; else psw &= (uint8_t)~PSW_AC;
        if (((~(value ^ (uint8_t)(imm + rhs))) & (value ^ cpu->acc) & 0x80u) != 0u) psw |= PSW_OV; else psw &= (uint8_t)~PSW_OV;
        cpu->psw = psw;
        break;
    case 0x38: /* MN_ADDC */ case 0x39: /* MN_ADDC */ case 0x3A: /* MN_ADDC */ case 0x3B: /* MN_ADDC */
    case 0x3C: /* MN_ADDC */ case 0x3D: /* MN_ADDC */ case 0x3E: /* MN_ADDC */ case 0x3F: /* MN_ADDC */
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        imm = cpu->iram[(uint8_t)(bank + (opcode & 0x07u))];
        value = cpu->acc;
        rhs = (uint8_t)((cpu->psw & PSW_CY) ? 1u : 0u);
        addr16 = (uint16_t)value + (uint16_t)imm + (uint16_t)rhs;
        cpu->acc = (uint8_t)addr16;
        psw = cpu->psw;
        if (addr16 > 0xFFu) psw |= PSW_CY; else psw &= (uint8_t)~PSW_CY;
        if (((value & 0x0Fu) + (imm & 0x0Fu) + rhs) > 0x0Fu) psw |= PSW_AC; else psw &= (uint8_t)~PSW_AC;
        if (((~(value ^ (uint8_t)(imm + rhs))) & (value ^ cpu->acc) & 0x80u) != 0u) psw |= PSW_OV; else psw &= (uint8_t)~PSW_OV;
        cpu->psw = psw;
        break;

    case 0x40: /* MN_JC */
        rel = (int8_t)cpu_fetch8(cpu);
        if (cpu->psw & PSW_CY) {
            cpu->pc = (uint16_t)(cpu->pc + rel);
        }
        break;

    case 0x42: /* MN_ORL */
        addr = cpu_fetch8(cpu);
        value = cpu_read_direct(cpu, addr);
        cpu_write_direct(cpu, addr, (uint8_t)(value | cpu->acc));
        break;
    case 0x43: /* MN_ORL */
        addr = cpu_fetch8(cpu);
        imm = cpu_fetch8(cpu);
        value = cpu_read_direct(cpu, addr);
        cpu_write_direct(cpu, addr, (uint8_t)(value | imm));
        break;
    case 0x44: /* MN_ORL */
        cpu->acc = (uint8_t)(cpu->acc | cpu_fetch8(cpu));
        break;
    case 0x45: /* MN_ORL */
        addr = cpu_fetch8(cpu);
        cpu->acc = (uint8_t)(cpu->acc | cpu_read_direct(cpu, addr));
        break;
    case 0x46: /* MN_ORL */ case 0x47: /* MN_ORL */
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        addr = cpu->iram[(uint8_t)(bank + (opcode & 0x01u))];
        cpu->acc = (uint8_t)(cpu->acc | cpu->iram[addr]);
        break;
    case 0x48: /* MN_ORL */ case 0x49: /* MN_ORL */ case 0x4A: /* MN_ORL */ case 0x4B: /* MN_ORL */
    case 0x4C: /* MN_ORL */ case 0x4D: /* MN_ORL */ case 0x4E: /* MN_ORL */ case 0x4F: /* MN_ORL */
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        cpu->acc = (uint8_t)(cpu->acc | cpu->iram[(uint8_t)(bank + (opcode & 0x07u))]);
        break;

    case 0x50: /* MN_JNC */
        rel = (int8_t)cpu_fetch8(cpu);
        if ((cpu->psw & PSW_CY) == 0) {
            cpu->pc = (uint16_t)(cpu->pc + rel);
        }
        break;

    case 0x52: /* MN_ANL */
        addr = cpu_fetch8(cpu);
        value = cpu_read_direct(cpu, addr);
        cpu_write_direct(cpu, addr, (uint8_t)(value & cpu->acc));
        break;
    case 0x53: /* MN_ANL */
        addr = cpu_fetch8(cpu);
        imm = cpu_fetch8(cpu);
        value = cpu_read_direct(cpu, addr);
        cpu_write_direct(cpu, addr, (uint8_t)(value & imm));
        break;
    case 0x54: /* MN_ANL */
        cpu->acc = (uint8_t)(cpu->acc & cpu_fetch8(cpu));
        break;
    case 0x55: /* MN_ANL */
        addr = cpu_fetch8(cpu);
        cpu->acc = (uint8_t)(cpu->acc & cpu_read_direct(cpu, addr));
        break;
    case 0x56: /* MN_ANL */ case 0x57: /* MN_ANL */
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        addr = cpu->iram[(uint8_t)(bank + (opcode & 0x01u))];
        cpu->acc = (uint8_t)(cpu->acc & cpu->iram[addr]);
        break;
    case 0x58: /* MN_ANL */ case 0x59: /* MN_ANL */ case 0x5A: /* MN_ANL */ case 0x5B: /* MN_ANL */
    case 0x5C: /* MN_ANL */ case 0x5D: /* MN_ANL */ case 0x5E: /* MN_ANL */ case 0x5F: /* MN_ANL */
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        cpu->acc = (uint8_t)(cpu->acc & cpu->iram[(uint8_t)(bank + (opcode & 0x07u))]);
        break;

    case 0x60: /* MN_JZ */
        rel = (int8_t)cpu_fetch8(cpu);
        if (cpu->acc == 0) {
            cpu->pc = (uint16_t)(cpu->pc + rel);
        }
        break;

    case 0x62: /* MN_XRL */
        addr = cpu_fetch8(cpu);
        value = cpu_read_direct(cpu, addr);
        cpu_write_direct(cpu, addr, (uint8_t)(value ^ cpu->acc));
        break;
    case 0x63: /* MN_XRL */
        addr = cpu_fetch8(cpu);
        imm = cpu_fetch8(cpu);
        value = cpu_read_direct(cpu, addr);
        cpu_write_direct(cpu, addr, (uint8_t)(value ^ imm));
        break;
    case 0x64: /* MN_XRL */
        cpu->acc = (uint8_t)(cpu->acc ^ cpu_fetch8(cpu));
        break;
    case 0x65: /* MN_XRL */
        addr = cpu_fetch8(cpu);
        cpu->acc = (uint8_t)(cpu->acc ^ cpu_read_direct(cpu, addr));
        break;
    case 0x66: /* MN_XRL */ case 0x67: /* MN_XRL */
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        addr = cpu->iram[(uint8_t)(bank + (opcode & 0x01u))];
        cpu->acc = (uint8_t)(cpu->acc ^ cpu->iram[addr]);
        break;
    case 0x68: /* MN_XRL */ case 0x69: /* MN_XRL */ case 0x6A: /* MN_XRL */ case 0x6B: /* MN_XRL */
    case 0x6C: /* MN_XRL */ case 0x6D: /* MN_XRL */ case 0x6E: /* MN_XRL */ case 0x6F: /* MN_XRL */
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        cpu->acc = (uint8_t)(cpu->acc ^ cpu->iram[(uint8_t)(bank + (opcode & 0x07u))]);
        break;

    case 0x70: /* MN_JNZ */
        rel = (int8_t)cpu_fetch8(cpu);
        if (cpu->acc != 0) {
            cpu->pc = (uint16_t)(cpu->pc + rel);
        }
        break;

    case 0x72: /* MN_ORL */
        bit = cpu_fetch8(cpu);
        if (cpu_read_bit(cpu, bit) != 0) {
            cpu->psw |= PSW_CY;
        }
        break;
    case 0x73: /* MN_JMPA_DPTR */
        cpu->pc = (uint16_t)(cpu->dptr + cpu->acc);
        break;
    case 0x74: /* MN_MOV */
        cpu->acc = cpu_fetch8(cpu);
        break;
    case 0x75: /* MN_MOV */
        addr = cpu_fetch8(cpu);
        imm = cpu_fetch8(cpu);
        cpu_write_direct(cpu, addr, imm);
        break;
    case 0x76: /* MN_MOV */ case 0x77: /* MN_MOV */
        imm = cpu_fetch8(cpu);
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        addr = cpu->iram[(uint8_t)(bank + (opcode & 0x01u))];
        cpu->iram[addr] = imm;
        break;
    case 0x78: /* MN_MOV */ case 0x79: /* MN_MOV */ case 0x7A: /* MN_MOV */ case 0x7B: /* MN_MOV */
    case 0x7C: /* MN_MOV */ case 0x7D: /* MN_MOV */ case 0x7E: /* MN_MOV */ case 0x7F: /* MN_MOV */
        imm = cpu_fetch8(cpu);
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        cpu->iram[(uint8_t)(bank + (opcode & 0x07u))] = imm;
        break;

    case 0x80: /* MN_SJMP */
        rel = (int8_t)cpu_fetch8(cpu);
        cpu->pc = (uint16_t)(cpu->pc + rel);
        break;

    case 0x82: /* MN_ANL */
        bit = cpu_fetch8(cpu);
        if (cpu_read_bit(cpu, bit) == 0) {
            cpu->psw &= (uint8_t)~PSW_CY;
        }
        break;
    case 0x83: /* MN_MOVC_A_PC */
        cpu->acc = cpu_code_read(cpu, (uint16_t)(cpu->pc + cpu->acc));
        break;
    case 0x84: /* MN_DIV */
        if (cpu->b == 0) {
            cpu->psw |= PSW_OV;
            cpu->psw &= (uint8_t)~PSW_CY;
        } else {
            value = cpu->acc;
            cpu->acc = (uint8_t)(value / cpu->b);
            cpu->b = (uint8_t)(value % cpu->b);
            cpu->psw &= (uint8_t)~PSW_OV;
            cpu->psw &= (uint8_t)~PSW_CY;
        }
        break;
    case 0x85: /* MN_MOV_DIRECT_DIRECT */
        rhs = cpu_fetch8(cpu);
        addr = cpu_fetch8(cpu);
        cpu_write_direct(cpu, addr, cpu_read_direct(cpu, rhs));
        break;
    case 0x86: /* MN_MOV */ case 0x87: /* MN_MOV */
        addr = cpu_fetch8(cpu);
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        rhs = cpu->iram[cpu->iram[(uint8_t)(bank + (opcode & 0x01u))]];
        cpu_write_direct(cpu, addr, rhs);
        break;
    case 0x88: /* MN_MOV */ case 0x89: /* MN_MOV */ case 0x8A: /* MN_MOV */ case 0x8B: /* MN_MOV */
    case 0x8C: /* MN_MOV */ case 0x8D: /* MN_MOV */ case 0x8E: /* MN_MOV */ case 0x8F: /* MN_MOV */
        addr = cpu_fetch8(cpu);
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        cpu_write_direct(cpu, addr, cpu->iram[(uint8_t)(bank + (opcode & 0x07u))]);
        break;

    case 0x90: /* MN_MOV_DPTR_IMM */
        cpu->dptr = cpu_fetch16(cpu);
        break;

    case 0x92: /* MN_MOV */
        bit = cpu_fetch8(cpu);
        cpu_write_bit(cpu, bit, (cpu->psw & PSW_CY) != 0);
        break;
    case 0x93: /* MN_MOVC_A_DPTR */
        cpu->acc = cpu_code_read(cpu, (uint16_t)(cpu->dptr + cpu->acc));
        break;
    case 0x94: /* MN_SUBB */
        imm = cpu_fetch8(cpu);
        value = cpu->acc;
        rhs = (uint8_t)(imm + ((cpu->psw & PSW_CY) ? 1u : 0u));
        cpu->acc = (uint8_t)(value - rhs);
        psw = cpu->psw;
        if (value < rhs) psw |= PSW_CY; else psw &= (uint8_t)~PSW_CY;
        if ((value & 0x0Fu) < ((imm & 0x0Fu) + ((cpu->psw & PSW_CY) ? 1u : 0u))) psw |= PSW_AC; else psw &= (uint8_t)~PSW_AC;
        if (((value ^ imm) & (value ^ cpu->acc) & 0x80u) != 0u) psw |= PSW_OV; else psw &= (uint8_t)~PSW_OV;
        cpu->psw = psw;
        break;
    case 0x95: /* MN_SUBB */
        addr = cpu_fetch8(cpu);
        imm = cpu_read_direct(cpu, addr);
        value = cpu->acc;
        rhs = (uint8_t)(imm + ((cpu->psw & PSW_CY) ? 1u : 0u));
        cpu->acc = (uint8_t)(value - rhs);
        psw = cpu->psw;
        if (value < rhs) psw |= PSW_CY; else psw &= (uint8_t)~PSW_CY;
        if ((value & 0x0Fu) < ((imm & 0x0Fu) + ((cpu->psw & PSW_CY) ? 1u : 0u))) psw |= PSW_AC; else psw &= (uint8_t)~PSW_AC;
        if (((value ^ imm) & (value ^ cpu->acc) & 0x80u) != 0u) psw |= PSW_OV; else psw &= (uint8_t)~PSW_OV;
        cpu->psw = psw;
        break;
    case 0x96: /* MN_SUBB */ case 0x97: /* MN_SUBB */
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        addr = cpu->iram[(uint8_t)(bank + (opcode & 0x01u))];
        imm = cpu->iram[addr];
        value = cpu->acc;
        rhs = (uint8_t)(imm + ((cpu->psw & PSW_CY) ? 1u : 0u));
        cpu->acc = (uint8_t)(value - rhs);
        psw = cpu->psw;
        if (value < rhs) psw |= PSW_CY; else psw &= (uint8_t)~PSW_CY;
        if ((value & 0x0Fu) < ((imm & 0x0Fu) + ((cpu->psw & PSW_CY) ? 1u : 0u))) psw |= PSW_AC; else psw &= (uint8_t)~PSW_AC;
        if (((value ^ imm) & (value ^ cpu->acc) & 0x80u) != 0u) psw |= PSW_OV; else psw &= (uint8_t)~PSW_OV;
        cpu->psw = psw;
        break;
    case 0x98: /* MN_SUBB */ case 0x99: /* MN_SUBB */ case 0x9A: /* MN_SUBB */ case 0x9B: /* MN_SUBB */
    case 0x9C: /* MN_SUBB */ case 0x9D: /* MN_SUBB */ case 0x9E: /* MN_SUBB */ case 0x9F: /* MN_SUBB */
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        imm = cpu->iram[(uint8_t)(bank + (opcode & 0x07u))];
        value = cpu->acc;
        rhs = (uint8_t)(imm + ((cpu->psw & PSW_CY) ? 1u : 0u));
        cpu->acc = (uint8_t)(value - rhs);
        psw = cpu->psw;
        if (value < rhs) psw |= PSW_CY; else psw &= (uint8_t)~PSW_CY;
        if ((value & 0x0Fu) < ((imm & 0x0Fu) + ((cpu->psw & PSW_CY) ? 1u : 0u))) psw |= PSW_AC; else psw &= (uint8_t)~PSW_AC;
        if (((value ^ imm) & (value ^ cpu->acc) & 0x80u) != 0u) psw |= PSW_OV; else psw &= (uint8_t)~PSW_OV;
        cpu->psw = psw;
        break;

    case 0xA0: /* MN_ORL */
        bit = cpu_fetch8(cpu);
        if (cpu_read_bit(cpu, bit) == 0) cpu->psw |= PSW_CY;
        break;
    case 0xA2: /* MN_MOV */
        bit = cpu_fetch8(cpu);
        if (cpu_read_bit(cpu, bit)) cpu->psw |= PSW_CY; else cpu->psw &= (uint8_t)~PSW_CY;
        break;
    case 0xA3: /* MN_INC */
        cpu->dptr++;
        break;
    case 0xA4: /* MN_MUL */
        addr16 = (uint16_t)cpu->acc * (uint16_t)cpu->b;
        cpu->acc = (uint8_t)(addr16 & 0xFFu);
        cpu->b = (uint8_t)(addr16 >> 8);
        cpu->psw &= (uint8_t)~PSW_CY;
        if (addr16 > 0xFFu) cpu->psw |= PSW_OV; else cpu->psw &= (uint8_t)~PSW_OV;
        break;
    case 0xA5: /* MN_INVALID */
        cpu->halted = true;
        cpu->halt_reason = "invalid opcode";
        break;
    case 0xA6: /* MN_MOV */ case 0xA7: /* MN_MOV */
        addr = cpu_fetch8(cpu);
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        idx = (uint8_t)(bank + (opcode & 0x01u));
        cpu->iram[cpu->iram[idx]] = cpu_read_direct(cpu, addr);
        break;
    case 0xA8: /* MN_MOV */ case 0xA9: /* MN_MOV */ case 0xAA: /* MN_MOV */ case 0xAB: /* MN_MOV */
    case 0xAC: /* MN_MOV */ case 0xAD: /* MN_MOV */ case 0xAE: /* MN_MOV */ case 0xAF: /* MN_MOV */
        addr = cpu_fetch8(cpu);
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        cpu->iram[(uint8_t)(bank + (opcode & 0x07u))] = cpu_read_direct(cpu, addr);
        break;

    case 0xB0: /* MN_ANL */
        bit = cpu_fetch8(cpu);
        if (cpu_read_bit(cpu, bit) != 0) cpu->psw &= (uint8_t)~PSW_CY;
        break;
    case 0xB2: /* MN_CPL */
        bit = cpu_fetch8(cpu);
        cpu_write_bit(cpu, bit, cpu_read_bit(cpu, bit) == 0);
        break;
    case 0xB3: /* MN_CPL */
        cpu->psw ^= PSW_CY;
        break;
    case 0xB4: /* MN_CJNE_A_IMM */
        imm = cpu_fetch8(cpu);
        rel = (int8_t)cpu_fetch8(cpu);
        value = cpu->acc;
        if (value < imm) cpu->psw |= PSW_CY; else cpu->psw &= (uint8_t)~PSW_CY;
        if (value != imm) cpu->pc = (uint16_t)(cpu->pc + rel);
        break;
    case 0xB5: /* MN_CJNE_A_DIRECT */
        addr = cpu_fetch8(cpu);
        rel = (int8_t)cpu_fetch8(cpu);
        imm = cpu_read_direct(cpu, addr);
        value = cpu->acc;
        if (value < imm) cpu->psw |= PSW_CY; else cpu->psw &= (uint8_t)~PSW_CY;
        if (value != imm) cpu->pc = (uint16_t)(cpu->pc + rel);
        break;
    case 0xB6: /* MN_CJNE_AT_RI_IMM */ case 0xB7: /* MN_CJNE_AT_RI_IMM */
        imm = cpu_fetch8(cpu);
        rel = (int8_t)cpu_fetch8(cpu);
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        value = cpu->iram[cpu->iram[(uint8_t)(bank + (opcode & 0x01u))]];
        if (value < imm) cpu->psw |= PSW_CY; else cpu->psw &= (uint8_t)~PSW_CY;
        if (value != imm) cpu->pc = (uint16_t)(cpu->pc + rel);
        break;
    case 0xB8: /* MN_CJNE_RN_IMM */ case 0xB9: /* MN_CJNE_RN_IMM */ case 0xBA: /* MN_CJNE_RN_IMM */ case 0xBB: /* MN_CJNE_RN_IMM */
    case 0xBC: /* MN_CJNE_RN_IMM */ case 0xBD: /* MN_CJNE_RN_IMM */ case 0xBE: /* MN_CJNE_RN_IMM */ case 0xBF: /* MN_CJNE_RN_IMM */
        imm = cpu_fetch8(cpu);
        rel = (int8_t)cpu_fetch8(cpu);
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        value = cpu->iram[(uint8_t)(bank + (opcode & 0x07u))];
        if (value < imm) cpu->psw |= PSW_CY; else cpu->psw &= (uint8_t)~PSW_CY;
        if (value != imm) cpu->pc = (uint16_t)(cpu->pc + rel);
        break;

    case 0xC0: /* MN_PUSH */
        addr = cpu_fetch8(cpu);
        cpu->sp++;
        cpu->iram[cpu->sp] = cpu_read_direct(cpu, addr);
        break;
    case 0xC2: /* MN_CLR */
        bit = cpu_fetch8(cpu);
        cpu_write_bit(cpu, bit, false);
        break;
    case 0xC3: /* MN_CLR */
        cpu->psw &= (uint8_t)~PSW_CY;
        break;
    case 0xC4: /* MN_SWAP */
        cpu->acc = (uint8_t)((cpu->acc << 4) | (cpu->acc >> 4));
        break;
    case 0xC5: /* MN_XCH */
        addr = cpu_fetch8(cpu);
        value = cpu->acc;
        cpu->acc = cpu_read_direct(cpu, addr);
        cpu_write_direct(cpu, addr, value);
        break;
    case 0xC6: /* MN_XCH */ case 0xC7: /* MN_XCH */
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        addr = cpu->iram[(uint8_t)(bank + (opcode & 0x01u))];
        value = cpu->acc;
        cpu->acc = cpu->iram[addr];
        cpu->iram[addr] = value;
        break;
    case 0xC8: /* MN_XCH */ case 0xC9: /* MN_XCH */ case 0xCA: /* MN_XCH */ case 0xCB: /* MN_XCH */
    case 0xCC: /* MN_XCH */ case 0xCD: /* MN_XCH */ case 0xCE: /* MN_XCH */ case 0xCF: /* MN_XCH */
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        idx = (uint8_t)(bank + (opcode & 0x07u));
        value = cpu->acc;
        cpu->acc = cpu->iram[idx];
        cpu->iram[idx] = value;
        break;

    case 0xD0: /* MN_POP */
        addr = cpu_fetch8(cpu);
        cpu_write_direct(cpu, addr, cpu->iram[cpu->sp]);
        cpu->sp--;
        break;
    case 0xD2: /* MN_SETB */
        bit = cpu_fetch8(cpu);
        cpu_write_bit(cpu, bit, true);
        break;
    case 0xD3: /* MN_SETB */
        cpu->psw |= PSW_CY;
        break;
    case 0xD4: /* MN_DA */
        value = cpu->acc;
        rhs = 0;
        if (((value & 0x0Fu) > 9u) || (cpu->psw & PSW_AC)) {
            rhs |= 0x06u;
        }
        if ((value > 0x99u) || (cpu->psw & PSW_CY)) {
            rhs |= 0x60u;
            cpu->psw |= PSW_CY;
        }
        cpu->acc = (uint8_t)(value + rhs);
        break;
    case 0xD5: /* MN_DJNZ */
        addr = cpu_fetch8(cpu);
        rel = (int8_t)cpu_fetch8(cpu);
        value = (uint8_t)(cpu_read_direct(cpu, addr) - 1u);
        cpu_write_direct(cpu, addr, value);
        if (value != 0) cpu->pc = (uint16_t)(cpu->pc + rel);
        break;
    case 0xD6: /* MN_XCHD */ case 0xD7: /* MN_XCHD */
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        addr = cpu->iram[(uint8_t)(bank + (opcode & 0x01u))];
        value = cpu->iram[addr];
        rhs = (uint8_t)(cpu->acc & 0x0Fu);
        cpu->acc = (uint8_t)((cpu->acc & 0xF0u) | (value & 0x0Fu));
        cpu->iram[addr] = (uint8_t)((value & 0xF0u) | rhs);
        break;
    case 0xD8: /* MN_DJNZ */ case 0xD9: /* MN_DJNZ */ case 0xDA: /* MN_DJNZ */ case 0xDB: /* MN_DJNZ */
    case 0xDC: /* MN_DJNZ */ case 0xDD: /* MN_DJNZ */ case 0xDE: /* MN_DJNZ */ case 0xDF: /* MN_DJNZ */
        rel = (int8_t)cpu_fetch8(cpu);
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        idx = (uint8_t)(bank + (opcode & 0x07u));
        cpu->iram[idx]--;
        if (cpu->iram[idx] != 0) cpu->pc = (uint16_t)(cpu->pc + rel);
        break;

    case 0xE0: /* MN_MOVX_A_DPTR */
        cpu->acc = cpu_xdata_read(cpu, cpu->dptr);
        break;
    case 0xE2: /* MN_MOVX_A_AT_RI */ case 0xE3: /* MN_MOVX_A_AT_RI */
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        addr = cpu->iram[(uint8_t)(bank + (opcode & 0x01u))];
        cpu->acc = cpu_xdata_read(cpu, addr);
        break;
    case 0xE4: /* MN_CLR */
        cpu->acc = 0;
        break;
    case 0xE5: /* MN_MOV */
        addr = cpu_fetch8(cpu);
        cpu->acc = cpu_read_direct(cpu, addr);
        break;
    case 0xE6: /* MN_MOV */ case 0xE7: /* MN_MOV */
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        addr = cpu->iram[(uint8_t)(bank + (opcode & 0x01u))];
        cpu->acc = cpu->iram[addr];
        break;
    case 0xE8: /* MN_MOV */ case 0xE9: /* MN_MOV */ case 0xEA: /* MN_MOV */ case 0xEB: /* MN_MOV */
    case 0xEC: /* MN_MOV */ case 0xED: /* MN_MOV */ case 0xEE: /* MN_MOV */ case 0xEF: /* MN_MOV */
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        cpu->acc = cpu->iram[(uint8_t)(bank + (opcode & 0x07u))];
        break;

    case 0xF0: /* MN_MOVX_DPTR_A */
        cpu_xdata_write(cpu, cpu->dptr, cpu->acc);
        break;
    case 0xF2: /* MN_MOVX_AT_RI_A */ case 0xF3: /* MN_MOVX_AT_RI_A */
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        addr = cpu->iram[(uint8_t)(bank + (opcode & 0x01u))];
        cpu_xdata_write(cpu, addr, cpu->acc);
        break;
    case 0xF4: /* MN_CPL */
        cpu->acc = (uint8_t)~cpu->acc;
        break;
    case 0xF5: /* MN_MOV */
        addr = cpu_fetch8(cpu);
        cpu_write_direct(cpu, addr, cpu->acc);
        break;
    case 0xF6: /* MN_MOV */ case 0xF7: /* MN_MOV */
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        addr = cpu->iram[(uint8_t)(bank + (opcode & 0x01u))];
        cpu->iram[addr] = cpu->acc;
        break;
    case 0xF8: /* MN_MOV */ case 0xF9: /* MN_MOV */ case 0xFA: /* MN_MOV */ case 0xFB: /* MN_MOV */
    case 0xFC: /* MN_MOV */ case 0xFD: /* MN_MOV */ case 0xFE: /* MN_MOV */ case 0xFF: /* MN_MOV */
        bank = (uint8_t)(((cpu->psw >> 3) & 0x03u) << 3);
        cpu->iram[(uint8_t)(bank + (opcode & 0x07u))] = cpu->acc;
        break;

    default:
        cpu->halted = true;
        cpu->halt_reason = "invalid opcode";
        break;
    }

    if (cpu->acc != acc_before) {
        uint8_t p = cpu->acc;
        p ^= (uint8_t)(p >> 4);
        p ^= (uint8_t)(p >> 2);
        p ^= (uint8_t)(p >> 1);
        if (p & 1u) {
            cpu->psw |= PSW_P;
        } else {
            cpu->psw &= (uint8_t)~PSW_P;
        }
    }

    if (cpu->trace_enabled && cpu->trace_fn) {
        cpu->trace_fn(cpu, pc_before, opcode, NULL, cpu->trace_user);
    }

    if (!cpu->halted) {
        cpu_check_interrupts(cpu);
    }

    return cpu->halted ? 0 : cycles;
}

uint64_t cpu_run(cpu_t *cpu, uint64_t max_steps)
{
    uint64_t steps = 0;
    while (!cpu->halted) {
        if (max_steps != 0 && steps >= max_steps) {
            break;
        }
        uint8_t cycles = cpu_step(cpu);
        if (cycles == 0) {
            if (cpu->idle && !cpu->power_down) {
                cycles = 1;
            } else {
                break;
            }
        }
        for (uint8_t i = 0; i < cpu->tick_hook_count; ++i) {
            const cpu_tick_entry_t *entry = &cpu->tick_hooks[i];
            if (entry->fn) {
                entry->fn(cpu, cycles, entry->user);
            }
        }
        if (cpu->idle && !cpu->power_down) {
            cpu_check_interrupts(cpu);
        }
        steps++;
    }
    return steps;
}

uint64_t cpu_run_timed(cpu_t *cpu,
                       uint64_t max_steps,
                       const timing_config_t *timing_cfg,
                       timing_state_t *timing_state,
                       const cpu_time_iface_t *time_iface)
{
    if (!timing_cfg || !timing_state) {
        return cpu_run(cpu, max_steps);
    }

    uint64_t steps = 0;
    uint64_t start_ns = 0;
    if (time_iface && time_iface->now_ns) {
        start_ns = time_iface->now_ns(time_iface->user);
    }

    while (!cpu->halted) {
        if (max_steps != 0 && steps >= max_steps) {
            break;
        }

        bool prof_enabled = g_run_timed_profiler.read_cycles && g_run_timed_profiler.on_sample;
        uint32_t t0 = 0;
        uint32_t t1 = 0;
        uint32_t t2 = 0;
        uint32_t t3 = 0;
        if (prof_enabled) {
            t0 = g_run_timed_profiler.read_cycles(g_run_timed_profiler.user);
        }

        uint8_t cycles = cpu_step(cpu);
        if (prof_enabled) {
            t1 = g_run_timed_profiler.read_cycles(g_run_timed_profiler.user);
        }

        if (cycles == 0) {
            if (cpu->idle && !cpu->power_down) {
                cycles = 1;
            } else {
                break;
            }
        }

        for (uint8_t i = 0; i < cpu->tick_hook_count; ++i) {
            const cpu_tick_entry_t *entry = &cpu->tick_hooks[i];
            if (entry->fn) {
                entry->fn(cpu, cycles, entry->user);
            }
        }
        if (prof_enabled) {
            t2 = g_run_timed_profiler.read_cycles(g_run_timed_profiler.user);
        }

        timing_step(timing_state, cycles);
        if (cpu->idle && !cpu->power_down) {
            cpu_check_interrupts(cpu);
        }
        steps++;

        if (time_iface && time_iface->now_ns && time_iface->sleep_ns) {
            uint64_t now_ns = time_iface->now_ns(time_iface->user);
            uint64_t target_ns = timing_cycles_to_ns(timing_cfg, timing_state->cycles_total);
            uint64_t elapsed_ns = now_ns - start_ns;
            if (target_ns > elapsed_ns) {
                time_iface->sleep_ns(target_ns - elapsed_ns, time_iface->user);
            }
        }

        if (prof_enabled) {
            t3 = g_run_timed_profiler.read_cycles(g_run_timed_profiler.user);
            g_run_timed_profiler.on_sample(t1 - t0,
                                           t2 - t1,
                                           t3 - t2,
                                           t3 - t0,
                                           cycles,
                                           g_run_timed_profiler.user);
        }
    }
    return steps;
}

void cpu_set_run_timed_profiler(const cpu_run_timed_profiler_t *profiler)
{
    if (!profiler) {
        g_run_timed_profiler.read_cycles = NULL;
        g_run_timed_profiler.on_sample = NULL;
        g_run_timed_profiler.user = NULL;
        return;
    }
    g_run_timed_profiler = *profiler;
}

void cpu_set_trace(cpu_t *cpu, bool enabled, cpu_trace_fn fn, void *user)
{
    cpu->trace_enabled = enabled;
    cpu->trace_fn = fn;
    cpu->trace_user = user;
}

void cpu_set_sfr_hook(cpu_t *cpu, uint8_t addr, sfr_read_hook read, sfr_write_hook write, void *user)
{
    if (addr < 0x80) {
        return;
    }
    uint8_t idx = (uint8_t)(addr - 0x80);
    cpu->sfr_hooks[idx].read = read;
    cpu->sfr_hooks[idx].write = write;
    cpu->sfr_hooks[idx].user = user;
}

void cpu_set_mem_ops(cpu_t *cpu, const cpu_mem_ops_t *ops, const void *user)
{
    if (ops) {
        cpu->mem_ops = *ops;
    } else {
        cpu->mem_ops.code_read = NULL;
        cpu->mem_ops.code_write = NULL;
        cpu->mem_ops.xdata_read = NULL;
        cpu->mem_ops.xdata_write = NULL;
    }
    cpu->mem_user = (void *)user;
}

void cpu_set_tick_hooks(cpu_t *cpu, const cpu_tick_entry_t *hooks, uint8_t count)
{
    if (!cpu) {
        return;
    }
    cpu->tick_hooks = hooks;
    cpu->tick_hook_count = hooks ? count : 0;
}

void cpu_set_int0_level(cpu_t *cpu, bool level)
{
    if (!cpu) {
        return;
    }
    uint8_t tcon = sfr_get(cpu, SFR_TCON);
    bool it0 = (tcon & TCON_IT0) != 0;
    if (it0) {
        if (cpu->int0_prev_level && !level) {
            tcon |= TCON_IE0;
        }
        cpu->int0_prev_level = level;
    } else {
        if (level) {
            tcon &= (uint8_t)~TCON_IE0;
        } else {
            tcon |= TCON_IE0;
        }
    }
    cpu->int0_level = level;
    sfr_set(cpu, SFR_TCON, tcon);
}

void cpu_set_int1_level(cpu_t *cpu, bool level)
{
    if (!cpu) {
        return;
    }
    uint8_t tcon = sfr_get(cpu, SFR_TCON);
    bool it1 = (tcon & TCON_IT1) != 0;
    if (it1) {
        if (cpu->int1_prev_level && !level) {
            tcon |= TCON_IE1;
        }
        cpu->int1_prev_level = level;
    } else {
        if (level) {
            tcon &= (uint8_t)~TCON_IE1;
        } else {
            tcon |= TCON_IE1;
        }
    }
    cpu->int1_level = level;
    sfr_set(cpu, SFR_TCON, tcon);
}

void cpu_on_reti(cpu_t *cpu)
{
    if (!cpu) {
        return;
    }
    if (cpu->isr_depth > 0) {
        cpu->isr_depth--;
        cpu->isr_prio = cpu->isr_stack[cpu->isr_depth];
        cpu->in_isr = true;
    } else {
        cpu->in_isr = false;
        cpu->isr_prio = 0;
    }
}

void cpu_poll_interrupts(cpu_t *cpu)
{
    cpu_check_interrupts(cpu);
}

void cpu_wake(cpu_t *cpu)
{
    if (!cpu) {
        return;
    }
    cpu->idle = false;
    cpu->power_down = false;
    uint8_t pcon = sfr_get(cpu, SFR_PCON);
    pcon &= (uint8_t)~(PCON_IDL | PCON_PD);
    sfr_set(cpu, SFR_PCON, pcon);
}

static void cpu_service_interrupt(cpu_t *cpu, int src, uint8_t prio, uint16_t vector)
{
    if (cpu->idle) {
        cpu->idle = false;
        uint8_t pcon = sfr_get(cpu, SFR_PCON);
        pcon &= (uint8_t)~PCON_IDL;
        sfr_set(cpu, SFR_PCON, pcon);
    }
    if (cpu->in_isr) {
        if (cpu->isr_depth < MCS51_ARRAY_LEN(cpu->isr_stack)) {
            cpu->isr_stack[cpu->isr_depth++] = cpu->isr_prio;
        }
    }
    cpu->in_isr = true;
    cpu->isr_prio = prio;

    uint16_t ret = cpu->pc;
    cpu->sp++;
    cpu->iram[cpu->sp] = (uint8_t)(ret & 0xFF);
    cpu->sp++;
    cpu->iram[cpu->sp] = (uint8_t)(ret >> 8);
    cpu->pc = vector;

    uint8_t tcon = sfr_get(cpu, SFR_TCON);
    switch (src) {
    case 0: { /* INT0 */
        if (tcon & TCON_IT0) {
            tcon &= (uint8_t)~TCON_IE0;
        }
        sfr_set(cpu, SFR_TCON, tcon);
        break; }
    case 1: { /* T0 */
        tcon &= (uint8_t)~TCON_TF0;
        sfr_set(cpu, SFR_TCON, tcon);
        break; }
    case 2: { /* INT1 */
        if (tcon & TCON_IT1) {
            tcon &= (uint8_t)~TCON_IE1;
        }
        sfr_set(cpu, SFR_TCON, tcon);
        break; }
    case 3: { /* T1 */
        tcon &= (uint8_t)~TCON_TF1;
        sfr_set(cpu, SFR_TCON, tcon);
        break; }
    case 4: { /* SERIAL */
        break; }
    case 5: { /* T2 */
        break; }
    default:
        break;
    }
}

static void cpu_check_interrupts(cpu_t *cpu)
{
    if (!cpu) {
        return;
    }
    if (cpu->power_down) {
        return;
    }

    uint8_t ie = sfr_get(cpu, SFR_IE);
    if ((ie & IE_EA) == 0) {
        return;
    }

    uint8_t ip = sfr_get(cpu, SFR_IP);
    uint8_t tcon = sfr_get(cpu, SFR_TCON);
    uint8_t scon = sfr_get(cpu, SFR_SCON);
    uint8_t t2con = sfr_get(cpu, SFR_T2CON);

    uint8_t best_prio = 0;
    uint16_t best_vec = 0;
    int best_src = -1;

    if ((ie & IE_EX0) && (tcon & TCON_IE0)) {
        uint8_t prio = (ip & IP_PX0) ? 1u : 0u;
        if (!cpu->in_isr || prio > cpu->isr_prio) {
            best_src = 0;
            best_prio = prio;
            best_vec = 0x0003;
        }
    }
    if ((ie & IE_ET0) && (tcon & TCON_TF0)) {
        uint8_t prio = (ip & IP_PT0) ? 1u : 0u;
        if ((!cpu->in_isr || prio > cpu->isr_prio) && (best_src < 0 || prio > best_prio)) {
            best_src = 1;
            best_prio = prio;
            best_vec = 0x000B;
        }
    }
    if ((ie & IE_EX1) && (tcon & TCON_IE1)) {
        uint8_t prio = (ip & IP_PX1) ? 1u : 0u;
        if ((!cpu->in_isr || prio > cpu->isr_prio) && (best_src < 0 || prio > best_prio)) {
            best_src = 2;
            best_prio = prio;
            best_vec = 0x0013;
        }
    }
    if ((ie & IE_ET1) && (tcon & TCON_TF1)) {
        uint8_t prio = (ip & IP_PT1) ? 1u : 0u;
        if ((!cpu->in_isr || prio > cpu->isr_prio) && (best_src < 0 || prio > best_prio)) {
            best_src = 3;
            best_prio = prio;
            best_vec = 0x001B;
        }
    }
    if ((ie & IE_ES) && (scon & (SCON_RI | SCON_TI))) {
        uint8_t prio = (ip & IP_PS) ? 1u : 0u;
        if ((!cpu->in_isr || prio > cpu->isr_prio) && (best_src < 0 || prio > best_prio)) {
            best_src = 4;
            best_prio = prio;
            best_vec = 0x0023;
        }
    }
    if ((ie & IE_ET2) && (t2con & (T2CON_TF2 | T2CON_EXF2))) {
        uint8_t prio = (ip & IP_PT2) ? 1u : 0u;
        if ((!cpu->in_isr || prio > cpu->isr_prio) && (best_src < 0 || prio > best_prio)) {
            best_src = 5;
            best_prio = prio;
            best_vec = 0x002B;
        }
    }

    if (best_src >= 0) {
        cpu_service_interrupt(cpu, best_src, best_prio, best_vec);
    }
}

static void cpu_apply_pcon(cpu_t *cpu, uint8_t value)
{
    if (!cpu) {
        return;
    }
    if (value & PCON_PD) {
        cpu->power_down = true;
        cpu->idle = false;
        return;
    }
    if (value & PCON_IDL) {
        cpu->idle = true;
    } else {
        cpu->idle = false;
    }
}

void cpu_set_carry(cpu_t *cpu, bool value)
{
    if (value) {
        cpu->psw |= PSW_CY;
    } else {
        cpu->psw &= (uint8_t)~PSW_CY;
    }
}

bool cpu_get_carry(const cpu_t *cpu)
{
    return (cpu->psw & PSW_CY) != 0;
}

void cpu_set_aux_carry(cpu_t *cpu, bool value)
{
    if (value) {
        cpu->psw |= PSW_AC;
    } else {
        cpu->psw &= (uint8_t)~PSW_AC;
    }
}

void cpu_set_overflow(cpu_t *cpu, bool value)
{
    if (value) {
        cpu->psw |= PSW_OV;
    } else {
        cpu->psw &= (uint8_t)~PSW_OV;
    }
}
