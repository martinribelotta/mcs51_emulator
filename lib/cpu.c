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

static void cpu_check_interrupts(cpu_t *cpu);
static void cpu_apply_pcon(cpu_t *cpu, uint8_t value);

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
    if (addr < 0x80) {
        return cpu->iram[addr];
    }
    uint8_t idx = (uint8_t)(addr - 0x80);
    if (cpu->sfr_hooks[idx].read) {
        void *user = cpu->sfr_hooks[idx].user;
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

uint8_t cpu_step(cpu_t *cpu)
{
    if (cpu->power_down || cpu->idle) {
        return 0;
    }
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
    }
    return steps;
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

static bool cpu_interrupt_pending(cpu_t *cpu, int src, uint8_t *out_prio, uint16_t *out_vector)
{
    if (cpu->power_down) {
        return false;
    }
    uint8_t ie = sfr_get(cpu, SFR_IE);
    uint8_t ip = sfr_get(cpu, SFR_IP);
    uint8_t tcon = sfr_get(cpu, SFR_TCON);
    uint8_t scon = sfr_get(cpu, SFR_SCON);

    if ((ie & IE_EA) == 0) {
        return false;
    }

    switch (src) {
    case 0: { /* INT0 */
        if ((ie & IE_EX0) && (tcon & TCON_IE0)) {
            *out_prio = (ip & IP_PX0) ? 1u : 0u;
            *out_vector = 0x0003;
            return true;
        }
        break; }
    case 1: { /* T0 */
        if ((ie & IE_ET0) && (tcon & TCON_TF0)) {
            *out_prio = (ip & IP_PT0) ? 1u : 0u;
            *out_vector = 0x000B;
            return true;
        }
        break; }
    case 2: { /* INT1 */
        if ((ie & IE_EX1) && (tcon & TCON_IE1)) {
            *out_prio = (ip & IP_PX1) ? 1u : 0u;
            *out_vector = 0x0013;
            return true;
        }
        break; }
    case 3: { /* T1 */
        if ((ie & IE_ET1) && (tcon & TCON_TF1)) {
            *out_prio = (ip & IP_PT1) ? 1u : 0u;
            *out_vector = 0x001B;
            return true;
        }
        break; }
    case 4: { /* SERIAL */
        if ((ie & IE_ES) && (scon & (SCON_RI | SCON_TI))) {
            *out_prio = (ip & IP_PS) ? 1u : 0u;
            *out_vector = 0x0023;
            return true;
        }
        break; }
    case 5: { /* T2 */
        if ((ie & IE_ET2) && (sfr_get(cpu, SFR_T2CON) & (T2CON_TF2 | T2CON_EXF2))) {
            *out_prio = (ip & IP_PT2) ? 1u : 0u;
            *out_vector = 0x002B;
            return true;
        }
        break; }
    default:
        break;
    }
    return false;
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

    uint8_t best_prio = 0;
    uint16_t best_vec = 0;
    int best_src = -1;

    for (int src = 0; src < 6; ++src) {
        uint8_t prio = 0;
        uint16_t vec = 0;
        if (!cpu_interrupt_pending(cpu, src, &prio, &vec)) {
            continue;
        }
        if (cpu->in_isr && prio <= cpu->isr_prio) {
            continue;
        }
        if (best_src < 0 || prio > best_prio) {
            best_src = src;
            best_prio = prio;
            best_vec = vec;
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
