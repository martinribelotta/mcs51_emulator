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
#else
#define MCS51_LIKELY(x)   (x)
#define MCS51_UNLIKELY(x) (x)
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

static void cpu_update_parity(cpu_t *cpu)
{
    uint8_t v = cpu->acc;
    v ^= (uint8_t)(v >> 4);
    v ^= (uint8_t)(v >> 2);
    v ^= (uint8_t)(v >> 1);
    if (v & 1u) {
        cpu->psw |= PSW_P;
    } else {
        cpu->psw &= (uint8_t)~PSW_P;
    }
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

uint8_t cpu_code_read(const cpu_t *cpu, uint16_t addr)
{
    if (cpu->mem_ops.code_read) {
        return cpu->mem_ops.code_read(cpu, addr, cpu->mem_user);
    }
    return 0xFF;
}

void cpu_code_write(cpu_t *cpu, uint16_t addr, uint8_t value)
{
    if (cpu->mem_ops.code_write) {
        cpu->mem_ops.code_write(cpu, addr, value, cpu->mem_user);
    }
}

uint8_t cpu_xdata_read(const cpu_t *cpu, uint16_t addr)
{
    if (cpu->mem_ops.xdata_read) {
        return cpu->mem_ops.xdata_read(cpu, addr, cpu->mem_user);
    }
    return 0xFF;
}

void cpu_xdata_write(cpu_t *cpu, uint16_t addr, uint8_t value)
{
    if (cpu->mem_ops.xdata_write) {
        cpu->mem_ops.xdata_write(cpu, addr, value, cpu->mem_user);
    }
}

static void bit_addr_to_byte_bit(uint8_t bit_addr, uint8_t *byte_addr, uint8_t *bit_pos)
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

static const addr_mode_t k_opcode_dst_modes[256] = {
#define X(op, mn, dst, src, size, cycles, name) [op] = dst,
    INSTR_LIST(X)
#undef X
};

static const addr_mode_t k_opcode_src_modes[256] = {
#define X(op, mn, dst, src, size, cycles, name) [op] = src,
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
    addr_mode_t dst_mode = k_opcode_dst_modes[opcode];
    addr_mode_t src_mode = k_opcode_src_modes[opcode];

    if (MCS51_UNLIKELY(cpu->trace_enabled && cpu->trace_fn)) {
        cpu->trace_fn(cpu, pc_before, opcode, opcode_name(opcode), cpu->trace_user);
    }

    switch (opcode) {
    case 0x04: /* INC A */
        cpu->acc = (uint8_t)(cpu->acc + 1u);
        goto finalize_step;
    case 0x05: { /* INC direct */
        uint8_t direct = cpu_fetch8(cpu);
        uint8_t value = (uint8_t)(cpu_read_direct(cpu, direct) + 1u);
        cpu_write_direct(cpu, direct, value);
        goto finalize_step; }
    case 0x14: /* DEC A */
        cpu->acc = (uint8_t)(cpu->acc - 1u);
        goto finalize_step;
    case 0x15: { /* DEC direct */
        uint8_t direct = cpu_fetch8(cpu);
        uint8_t value = (uint8_t)(cpu_read_direct(cpu, direct) - 1u);
        cpu_write_direct(cpu, direct, value);
        goto finalize_step; }
    case 0x24: { /* ADD A,#imm */
        cpu_add_a_fast(cpu, cpu_fetch8(cpu));
        goto finalize_step; }
    case 0x60: { /* JZ rel */
        int8_t rel = (int8_t)cpu_fetch8(cpu);
        if (cpu->acc == 0) {
            cpu->pc = (uint16_t)(cpu->pc + rel);
        }
        goto finalize_step; }
    case 0x70: { /* JNZ rel */
        int8_t rel = (int8_t)cpu_fetch8(cpu);
        if (cpu->acc != 0) {
            cpu->pc = (uint16_t)(cpu->pc + rel);
        }
        goto finalize_step; }
    case 0x74: /* MOV A,#imm */
        cpu->acc = cpu_fetch8(cpu);
        goto finalize_step;
    case 0x75: { /* MOV direct,#imm */
        uint8_t direct = cpu_fetch8(cpu);
        cpu_write_direct(cpu, direct, cpu_fetch8(cpu));
        goto finalize_step; }
    case 0x80: { /* SJMP rel */
        int8_t rel = (int8_t)cpu_fetch8(cpu);
        cpu->pc = (uint16_t)(cpu->pc + rel);
        goto finalize_step; }
    case 0xE4: /* CLR A */
        cpu->acc = 0;
        goto finalize_step;
    case 0xE5: { /* MOV A,direct */
        uint8_t direct = cpu_fetch8(cpu);
        cpu->acc = cpu_read_direct(cpu, direct);
        goto finalize_step; }
    case 0xF4: /* CPL A */
        cpu->acc = (uint8_t)~cpu->acc;
        goto finalize_step;
    case 0xF5: { /* MOV direct,A */
        uint8_t direct = cpu_fetch8(cpu);
        cpu_write_direct(cpu, direct, cpu->acc);
        goto finalize_step; }
    default:
        break;
    }

    if ((opcode & 0xF8u) == 0x08u) { /* INC Rn */
        uint8_t idx = (uint8_t)(cpu_reg_bank_base_fast(cpu) + (opcode & 0x07u));
        cpu->iram[idx] = (uint8_t)(cpu->iram[idx] + 1u);
        goto finalize_step;
    }
    if ((opcode & 0xF8u) == 0x18u) { /* DEC Rn */
        uint8_t idx = (uint8_t)(cpu_reg_bank_base_fast(cpu) + (opcode & 0x07u));
        cpu->iram[idx] = (uint8_t)(cpu->iram[idx] - 1u);
        goto finalize_step;
    }
    if ((opcode & 0xF8u) == 0x28u) { /* ADD A,Rn */
        uint8_t idx = (uint8_t)(cpu_reg_bank_base_fast(cpu) + (opcode & 0x07u));
        cpu_add_a_fast(cpu, cpu->iram[idx]);
        goto finalize_step;
    }
    if ((opcode & 0xF8u) == 0x78u) { /* MOV Rn,#imm */
        uint8_t idx = (uint8_t)(cpu_reg_bank_base_fast(cpu) + (opcode & 0x07u));
        cpu->iram[idx] = cpu_fetch8(cpu);
        goto finalize_step;
    }
    if ((opcode & 0xF8u) == 0x88u) { /* MOV direct,Rn */
        uint8_t idx = (uint8_t)(cpu_reg_bank_base_fast(cpu) + (opcode & 0x07u));
        uint8_t direct = cpu_fetch8(cpu);
        cpu_write_direct(cpu, direct, cpu->iram[idx]);
        goto finalize_step;
    }
    if ((opcode & 0xF8u) == 0xA8u) { /* MOV Rn,direct */
        uint8_t idx = (uint8_t)(cpu_reg_bank_base_fast(cpu) + (opcode & 0x07u));
        uint8_t direct = cpu_fetch8(cpu);
        cpu->iram[idx] = cpu_read_direct(cpu, direct);
        goto finalize_step;
    }
    if ((opcode & 0xF8u) == 0xD8u) { /* DJNZ Rn,rel */
        uint8_t idx = (uint8_t)(cpu_reg_bank_base_fast(cpu) + (opcode & 0x07u));
        int8_t rel = (int8_t)cpu_fetch8(cpu);
        uint8_t value = (uint8_t)(cpu->iram[idx] - 1u);
        cpu->iram[idx] = value;
        if (value != 0u) {
            cpu->pc = (uint16_t)(cpu->pc + rel);
        }
        goto finalize_step;
    }
    if ((opcode & 0xF8u) == 0xE8u) { /* MOV A,Rn */
        uint8_t idx = (uint8_t)(cpu_reg_bank_base_fast(cpu) + (opcode & 0x07u));
        cpu->acc = cpu->iram[idx];
        goto finalize_step;
    }
    if ((opcode & 0xF8u) == 0xF8u) { /* MOV Rn,A */
        uint8_t idx = (uint8_t)(cpu_reg_bank_base_fast(cpu) + (opcode & 0x07u));
        cpu->iram[idx] = cpu->acc;
        goto finalize_step;
    }

#if defined(__GNUC__) || defined(__clang__)
#define X(op, mn, dst, src, size, cycles, name) [op] = &&LBL_##mn,
    static void *const k_dispatch[256] = {
        INSTR_LIST(X)
    };
#undef X

    goto *k_dispatch[opcode];

LBL_MN_INVALID:
    cpu->halted = true;
    cpu->halt_reason = "UNDEFINED";
    goto finalize_step;

LBL_MN_NOP:
    exec_nop(cpu);
    goto finalize_step;
LBL_MN_MOV: {
    op_t dst = fetch_operand(cpu, dst_mode, opcode);
    op_t src = fetch_operand(cpu, src_mode, opcode);
    exec_mov(cpu, dst, src);
    goto finalize_step; }
LBL_MN_MOV_DIRECT_DIRECT: {
    op_t src = fetch_operand(cpu, AM_DIRECT, opcode);
    op_t dst = fetch_operand(cpu, AM_DIRECT, opcode);
    exec_mov_direct_direct(cpu, src, dst);
    goto finalize_step; }
LBL_MN_MOV_DPTR_IMM: {
    uint16_t value = cpu_fetch16(cpu);
    exec_mov_dptr_imm(cpu, value);
    goto finalize_step; }
LBL_MN_ADD: {
    op_t src = fetch_operand(cpu, src_mode, opcode);
    exec_add(cpu, src);
    goto finalize_step; }
LBL_MN_ADDC: {
    op_t src = fetch_operand(cpu, src_mode, opcode);
    exec_addc(cpu, src);
    goto finalize_step; }
LBL_MN_SUBB: {
    op_t src = fetch_operand(cpu, src_mode, opcode);
    exec_subb(cpu, src);
    goto finalize_step; }
LBL_MN_ANL: {
    op_t dst = fetch_operand(cpu, dst_mode, opcode);
    op_t src = fetch_operand(cpu, src_mode, opcode);
    exec_anl(cpu, dst, src);
    goto finalize_step; }
LBL_MN_ORL: {
    op_t dst = fetch_operand(cpu, dst_mode, opcode);
    op_t src = fetch_operand(cpu, src_mode, opcode);
    exec_orl(cpu, dst, src);
    goto finalize_step; }
LBL_MN_XRL: {
    op_t dst = fetch_operand(cpu, dst_mode, opcode);
    op_t src = fetch_operand(cpu, src_mode, opcode);
    exec_xrl(cpu, dst, src);
    goto finalize_step; }
LBL_MN_DA:
    exec_da(cpu);
    goto finalize_step;
LBL_MN_MUL:
    exec_mul(cpu);
    goto finalize_step;
LBL_MN_DIV:
    exec_div(cpu);
    goto finalize_step;
LBL_MN_RR:
    exec_rr(cpu);
    goto finalize_step;
LBL_MN_RRC:
    exec_rrc(cpu);
    goto finalize_step;
LBL_MN_RL:
    exec_rl(cpu);
    goto finalize_step;
LBL_MN_RLC:
    exec_rlc(cpu);
    goto finalize_step;
LBL_MN_SWAP:
    exec_swap(cpu);
    goto finalize_step;
LBL_MN_INC: {
    op_t dst = fetch_operand(cpu, dst_mode, opcode);
    exec_inc(cpu, dst);
    goto finalize_step; }
LBL_MN_DEC: {
    op_t dst = fetch_operand(cpu, dst_mode, opcode);
    exec_dec(cpu, dst);
    goto finalize_step; }
LBL_MN_JC: {
    op_t rel = fetch_operand(cpu, dst_mode, opcode);
    exec_jc(cpu, rel);
    goto finalize_step; }
LBL_MN_JNC: {
    op_t rel = fetch_operand(cpu, dst_mode, opcode);
    exec_jnc(cpu, rel);
    goto finalize_step; }
LBL_MN_JZ: {
    op_t rel = fetch_operand(cpu, dst_mode, opcode);
    exec_jz(cpu, rel);
    goto finalize_step; }
LBL_MN_JNZ: {
    op_t rel = fetch_operand(cpu, dst_mode, opcode);
    exec_jnz(cpu, rel);
    goto finalize_step; }
LBL_MN_JB: {
    op_t bit = fetch_operand(cpu, dst_mode, opcode);
    op_t rel = fetch_operand(cpu, src_mode, opcode);
    exec_jb(cpu, bit, rel);
    goto finalize_step; }
LBL_MN_JBC: {
    op_t bit = fetch_operand(cpu, dst_mode, opcode);
    op_t rel = fetch_operand(cpu, src_mode, opcode);
    exec_jbc(cpu, bit, rel);
    goto finalize_step; }
LBL_MN_JNB: {
    op_t bit = fetch_operand(cpu, dst_mode, opcode);
    op_t rel = fetch_operand(cpu, src_mode, opcode);
    exec_jnb(cpu, bit, rel);
    goto finalize_step; }
LBL_MN_DJNZ: {
    op_t dst = fetch_operand(cpu, dst_mode, opcode);
    op_t rel = fetch_operand(cpu, src_mode, opcode);
    exec_djnz(cpu, dst, rel);
    goto finalize_step; }
LBL_MN_CJNE_A_IMM: {
    op_t rhs = fetch_operand(cpu, AM_IMM8, opcode);
    op_t rel = fetch_operand(cpu, AM_REL8, opcode);
    exec_cjne(cpu, (op_t){ AM_A, 0 }, rhs, rel);
    goto finalize_step; }
LBL_MN_CJNE_A_DIRECT: {
    op_t rhs = fetch_operand(cpu, AM_DIRECT, opcode);
    op_t rel = fetch_operand(cpu, AM_REL8, opcode);
    exec_cjne(cpu, (op_t){ AM_A, 0 }, rhs, rel);
    goto finalize_step; }
LBL_MN_CJNE_AT_RI_IMM: {
    op_t lhs = fetch_operand(cpu, AM_AT_RI, opcode);
    op_t rhs = fetch_operand(cpu, AM_IMM8, opcode);
    op_t rel = fetch_operand(cpu, AM_REL8, opcode);
    exec_cjne(cpu, lhs, rhs, rel);
    goto finalize_step; }
LBL_MN_CJNE_RN_IMM: {
    op_t lhs = fetch_operand(cpu, AM_RN, opcode);
    op_t rhs = fetch_operand(cpu, AM_IMM8, opcode);
    op_t rel = fetch_operand(cpu, AM_REL8, opcode);
    exec_cjne(cpu, lhs, rhs, rel);
    goto finalize_step; }
LBL_MN_JMPA_DPTR:
    exec_jmpa_dptr(cpu);
    goto finalize_step;
LBL_MN_SJMP: {
    op_t rel = fetch_operand(cpu, dst_mode, opcode);
    exec_sjmp(cpu, rel);
    goto finalize_step; }
LBL_MN_AJMP: {
    op_t addr11 = fetch_operand(cpu, dst_mode, opcode);
    exec_ajmp(cpu, addr11);
    goto finalize_step; }
LBL_MN_ACALL: {
    op_t addr11 = fetch_operand(cpu, dst_mode, opcode);
    exec_acall(cpu, addr11);
    goto finalize_step; }
LBL_MN_LJMP: {
    op_t addr16 = fetch_operand(cpu, dst_mode, opcode);
    exec_ljmp(cpu, addr16);
    goto finalize_step; }
LBL_MN_LCALL: {
    op_t addr16 = fetch_operand(cpu, dst_mode, opcode);
    exec_lcall(cpu, addr16);
    goto finalize_step; }
LBL_MN_SETB: {
    op_t dst = fetch_operand(cpu, dst_mode, opcode);
    exec_setb(cpu, dst);
    goto finalize_step; }
LBL_MN_CLR: {
    op_t dst = fetch_operand(cpu, dst_mode, opcode);
    exec_clr(cpu, dst);
    goto finalize_step; }
LBL_MN_CPL: {
    op_t dst = fetch_operand(cpu, dst_mode, opcode);
    exec_cpl(cpu, dst);
    goto finalize_step; }
LBL_MN_MOVC_A_DPTR:
    exec_movc_a_dptr(cpu);
    goto finalize_step;
LBL_MN_MOVC_A_PC:
    exec_movc_a_pc(cpu);
    goto finalize_step;
LBL_MN_MOVX_A_DPTR:
    exec_movx_a_dptr(cpu);
    goto finalize_step;
LBL_MN_MOVX_DPTR_A:
    exec_movx_dptr_a(cpu);
    goto finalize_step;
LBL_MN_MOVX_A_AT_RI: {
    op_t dst = fetch_operand(cpu, dst_mode, opcode);
    exec_movx_a_at_ri(cpu, dst);
    goto finalize_step; }
LBL_MN_MOVX_AT_RI_A: {
    op_t dst = fetch_operand(cpu, dst_mode, opcode);
    exec_movx_at_ri_a(cpu, dst);
    goto finalize_step; }
LBL_MN_XCH: {
    op_t dst = fetch_operand(cpu, dst_mode, opcode);
    exec_xch(cpu, dst);
    goto finalize_step; }
LBL_MN_XCHD: {
    op_t dst = fetch_operand(cpu, dst_mode, opcode);
    exec_xchd(cpu, dst);
    goto finalize_step; }
LBL_MN_RET:
    exec_ret(cpu);
    goto finalize_step;
LBL_MN_RETI:
    exec_reti(cpu);
    goto finalize_step;
LBL_MN_PUSH: {
    op_t dst = fetch_operand(cpu, dst_mode, opcode);
    exec_push(cpu, dst);
    goto finalize_step; }
LBL_MN_POP: {
    op_t dst = fetch_operand(cpu, dst_mode, opcode);
    exec_pop(cpu, dst);
    goto finalize_step; }
#else
    const instr_desc_t *desc = decode_ptr(opcode);
    if (MCS51_UNLIKELY(desc->mnemonic == MN_INVALID)) {
        cpu->halted = true;
        cpu->halt_reason = "UNDEFINED";
        goto finalize_step;
    }
#endif

finalize_step:
    if (!cpu->halted) {
        if (cpu->acc != acc_before) {
            cpu_update_parity(cpu);
        }
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
