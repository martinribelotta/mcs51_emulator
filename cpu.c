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
#define SFR_PSW  0xD0
#define SFR_ACC  0xE0
#define SFR_B    0xF0

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
    memcpy(reset.sfr, cpu->sfr, sizeof(cpu->sfr));
    memcpy(reset.sfr_hooks, cpu->sfr_hooks, sizeof(cpu->sfr_hooks));
    reset.sfr_user = cpu->sfr_user;
    reset.mem_ops = cpu->mem_ops;
    reset.mem_user = cpu->mem_user;
    reset.tick_fn = cpu->tick_fn;
    reset.tick_user = cpu->tick_user;
    *cpu = reset;
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
    if (addr < 0x80) {
        return cpu->iram[addr];
    }
    uint8_t idx = (uint8_t)(addr - 0x80);
    if (cpu->sfr_hooks[idx].read) {
        void *user = cpu->sfr_hooks[idx].user ? cpu->sfr_hooks[idx].user : cpu->sfr_user;
        return cpu->sfr_hooks[idx].read(cpu, addr, user);
    }
    return cpu->sfr[idx];
}

void cpu_write_direct(cpu_t *cpu, uint8_t addr, uint8_t value)
{
    if (addr < 0x80) {
        cpu->iram[addr] = value;
        return;
    }
    uint8_t idx = (uint8_t)(addr - 0x80);
    if (cpu->sfr_hooks[idx].write) {
        void *user = cpu->sfr_hooks[idx].user ? cpu->sfr_hooks[idx].user : cpu->sfr_user;
        cpu->sfr_hooks[idx].write(cpu, addr, value, user);
        return;
    }
    cpu->sfr[idx] = value;
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

uint8_t cpu_step(cpu_t *cpu)
{
    if (cpu->halted) {
        return 0;
    }

    uint16_t pc_before = cpu->pc;
    uint8_t opcode = cpu_fetch8(cpu);
    cpu->last_opcode = opcode;

    instr_desc_t desc = decode(opcode);
    if (desc.mnemonic == MN_INVALID) {
        cpu->halted = true;
        cpu->halt_reason = "UNDEFINED";
        return 0;
    }
    uint8_t cycles = desc.cycles;

    if (cpu->trace_enabled && cpu->trace_fn) {
        cpu->trace_fn(cpu, pc_before, opcode, opcode_name(opcode), cpu->trace_user);
    }

    if (desc.mnemonic == MN_MOV_DIRECT_DIRECT) {
        op_t src = fetch_operand(cpu, AM_DIRECT, opcode);
        op_t dst = fetch_operand(cpu, AM_DIRECT, opcode);
        exec_mov_direct_direct(cpu, src, dst);
        return cpu->halted ? 0 : cycles;
    }

    if (desc.mnemonic == MN_MOV_DPTR_IMM) {
        uint16_t value = cpu_fetch16(cpu);
        exec_mov_dptr_imm(cpu, value);
        return cpu->halted ? 0 : cycles;
    }

    if (desc.mnemonic == MN_CJNE_A_IMM ||
        desc.mnemonic == MN_CJNE_A_DIRECT ||
        desc.mnemonic == MN_CJNE_AT_RI_IMM ||
        desc.mnemonic == MN_CJNE_RN_IMM) {
        switch (desc.mnemonic) {
        case MN_CJNE_A_IMM: {
            op_t rhs = fetch_operand(cpu, AM_IMM8, opcode);
            op_t rel = fetch_operand(cpu, AM_REL8, opcode);
            exec_cjne(cpu, (op_t){ AM_A, 0 }, rhs, rel);
            break; }
        case MN_CJNE_A_DIRECT: {
            op_t rhs = fetch_operand(cpu, AM_DIRECT, opcode);
            op_t rel = fetch_operand(cpu, AM_REL8, opcode);
            exec_cjne(cpu, (op_t){ AM_A, 0 }, rhs, rel);
            break; }
        case MN_CJNE_AT_RI_IMM: {
            op_t lhs = fetch_operand(cpu, AM_AT_RI, opcode);
            op_t rhs = fetch_operand(cpu, AM_IMM8, opcode);
            op_t rel = fetch_operand(cpu, AM_REL8, opcode);
            exec_cjne(cpu, lhs, rhs, rel);
            break; }
        case MN_CJNE_RN_IMM: {
            op_t lhs = fetch_operand(cpu, AM_RN, opcode);
            op_t rhs = fetch_operand(cpu, AM_IMM8, opcode);
            op_t rel = fetch_operand(cpu, AM_REL8, opcode);
            exec_cjne(cpu, lhs, rhs, rel);
            break; }
        default:
            cpu->halted = true;
            break;
        }
        return cpu->halted ? 0 : cycles;
    }

    op_t dst = fetch_operand(cpu, desc.dst_mode, opcode);
    op_t src = fetch_operand(cpu, desc.src_mode, opcode);

    switch (desc.mnemonic) {
    case MN_NOP:
        exec_nop(cpu);
        break;
    case MN_MOV:
        exec_mov(cpu, dst, src);
        break;
    case MN_SUBB:
        exec_subb(cpu, src);
        break;
    case MN_ADD:
        exec_add(cpu, src);
        break;
    case MN_ADDC:
        exec_addc(cpu, src);
        break;
    case MN_ANL:
        exec_anl(cpu, dst, src);
        break;
    case MN_ORL:
        exec_orl(cpu, dst, src);
        break;
    case MN_XRL:
        exec_xrl(cpu, dst, src);
        break;
    case MN_DA:
        exec_da(cpu);
        break;
    case MN_MUL:
        exec_mul(cpu);
        break;
    case MN_DIV:
        exec_div(cpu);
        break;
    case MN_RR:
        exec_rr(cpu);
        break;
    case MN_RRC:
        exec_rrc(cpu);
        break;
    case MN_RL:
        exec_rl(cpu);
        break;
    case MN_RLC:
        exec_rlc(cpu);
        break;
    case MN_SWAP:
        exec_swap(cpu);
        break;
    case MN_INC:
        exec_inc(cpu, dst);
        break;
    case MN_DEC:
        exec_dec(cpu, dst);
        break;
    case MN_JC:
        exec_jc(cpu, dst);
        break;
    case MN_JNC:
        exec_jnc(cpu, dst);
        break;
    case MN_JZ:
        exec_jz(cpu, dst);
        break;
    case MN_JNZ:
        exec_jnz(cpu, dst);
        break;
    case MN_JB:
        exec_jb(cpu, dst, src);
        break;
    case MN_JBC:
        exec_jbc(cpu, dst, src);
        break;
    case MN_JNB:
        exec_jnb(cpu, dst, src);
        break;
    case MN_DJNZ:
        exec_djnz(cpu, dst, src);
        break;
    case MN_JMPA_DPTR:
        exec_jmpa_dptr(cpu);
        break;
    case MN_SJMP:
        exec_sjmp(cpu, dst);
        break;
    case MN_AJMP:
        exec_ajmp(cpu, dst);
        break;
    case MN_ACALL:
        exec_acall(cpu, dst);
        break;
    case MN_LJMP:
        exec_ljmp(cpu, dst);
        break;
    case MN_LCALL:
        exec_lcall(cpu, dst);
        break;
    case MN_SETB:
        exec_setb(cpu, dst);
        break;
    case MN_CLR:
        exec_clr(cpu, dst);
        break;
    case MN_CPL:
        exec_cpl(cpu, dst);
        break;
    case MN_MOVC_A_DPTR:
        exec_movc_a_dptr(cpu);
        break;
    case MN_MOVC_A_PC:
        exec_movc_a_pc(cpu);
        break;
    case MN_MOVX_A_DPTR:
        exec_movx_a_dptr(cpu);
        break;
    case MN_MOVX_DPTR_A:
        exec_movx_dptr_a(cpu);
        break;
    case MN_MOVX_A_AT_RI:
        exec_movx_a_at_ri(cpu, dst);
        break;
    case MN_MOVX_AT_RI_A:
        exec_movx_at_ri_a(cpu, dst);
        break;
    case MN_XCH:
        exec_xch(cpu, dst);
        break;
    case MN_XCHD:
        exec_xchd(cpu, dst);
        break;
    case MN_RET:
        exec_ret(cpu);
        break;
    case MN_RETI:
        exec_reti(cpu);
        break;
    case MN_PUSH:
        exec_push(cpu, dst);
        break;
    case MN_POP:
        exec_pop(cpu, dst);
        break;
    default:
        cpu->halted = true;
        break;
    }
    if (!cpu->halted) {
        cpu_update_parity(cpu);
    }
    return cpu->halted ? 0 : cycles;
}

void cpu_run(cpu_t *cpu, uint64_t max_steps)
{
    uint64_t steps = 0;
    while (!cpu->halted) {
        if (max_steps != 0 && steps >= max_steps) {
            break;
        }
        uint8_t cycles = cpu_step(cpu);
        if (cycles == 0) {
            break;
        }
        if (cpu->tick_fn) {
            cpu->tick_fn(cpu, cycles, cpu->tick_user);
        }
        steps++;
    }
}

void cpu_run_timed(cpu_t *cpu,
                   uint64_t max_steps,
                   const timing_config_t *timing_cfg,
                   timing_state_t *timing_state,
                   const cpu_time_iface_t *time_iface)
{
    if (!timing_cfg || !timing_state) {
        cpu_run(cpu, max_steps);
        return;
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
        uint8_t cycles = cpu_step(cpu);
        if (cycles == 0) {
            break;
        }
        if (cpu->tick_fn) {
            cpu->tick_fn(cpu, cycles, cpu->tick_user);
        }
        timing_step(timing_state, cycles);
        steps++;

        if (time_iface && time_iface->now_ns && time_iface->sleep_ns) {
            uint64_t now_ns = time_iface->now_ns(time_iface->user);
            uint64_t target_ns = timing_cycles_to_ns(timing_cfg, timing_state->cycles_total);
            uint64_t elapsed_ns = now_ns - start_ns;
            if (target_ns > elapsed_ns) {
                time_iface->sleep_ns(target_ns - elapsed_ns, time_iface->user);
            }
        }
    }
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

void cpu_set_sfr_user(cpu_t *cpu, void *user)
{
    cpu->sfr_user = user;
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

void cpu_set_tick_hook(cpu_t *cpu, cpu_tick_fn fn, void *user)
{
    cpu->tick_fn = fn;
    cpu->tick_user = user;
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
