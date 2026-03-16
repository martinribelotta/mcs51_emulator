#include "cpu.h"
#include "decoder.h"
#include "instr_impl.h"
#include "instr_list.h"

#include <string.h>

#define PSW_CY 0x80
#define PSW_AC 0x40
#define PSW_OV 0x04

static uint8_t default_sfr_read(cpu_t *cpu, uint8_t addr)
{
    return cpu->sfr[addr - 0x80];
}

static void default_sfr_write(cpu_t *cpu, uint8_t addr, uint8_t value)
{
    cpu->sfr[addr - 0x80] = value;
}

void cpu_init(cpu_t *cpu)
{
    memset(cpu, 0, sizeof(*cpu));
    cpu->sfr_read = default_sfr_read;
    cpu->sfr_write = default_sfr_write;
    cpu_reset(cpu);
}

void cpu_reset(cpu_t *cpu)
{
    cpu->acc = 0;
    cpu->b = 0;
    cpu->psw = 0;
    cpu->sp = 0x07;
    cpu->dptr = 0;
    cpu->pc = 0;
    cpu->halted = false;
    cpu->last_opcode = 0x00;
    cpu->trace_enabled = false;
    cpu->trace_fn = NULL;
    cpu->trace_user = NULL;
}

uint8_t cpu_fetch8(cpu_t *cpu)
{
    return cpu->code[cpu->pc++];
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
    if (cpu->sfr_read) {
        return cpu->sfr_read(cpu, addr);
    }
    return cpu->sfr[addr - 0x80];
}

void cpu_write_direct(cpu_t *cpu, uint8_t addr, uint8_t value)
{
    if (addr < 0x80) {
        cpu->iram[addr] = value;
        return;
    }
    if (cpu->sfr_write) {
        cpu->sfr_write(cpu, addr, value);
        return;
    }
    cpu->sfr[addr - 0x80] = value;
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

bool cpu_step(cpu_t *cpu)
{
    if (cpu->halted) {
        return false;
    }

    uint16_t pc_before = cpu->pc;
    uint8_t opcode = cpu_fetch8(cpu);
    cpu->last_opcode = opcode;

    instr_desc_t desc = decode(opcode);
    if (desc.mnemonic == MN_INVALID) {
        cpu->halted = true;
        return false;
    }

    if (cpu->trace_enabled && cpu->trace_fn) {
        cpu->trace_fn(cpu, pc_before, opcode, opcode_name(opcode), cpu->trace_user);
    }

    if (desc.mnemonic == MN_MOV_DIRECT_DIRECT) {
        op_t src = fetch_operand(cpu, AM_DIRECT, opcode);
        op_t dst = fetch_operand(cpu, AM_DIRECT, opcode);
        exec_mov_direct_direct(cpu, src, dst);
        return !cpu->halted;
    }

    if (desc.mnemonic == MN_MOV_DPTR_IMM) {
        uint16_t value = cpu_fetch16(cpu);
        exec_mov_dptr_imm(cpu, value);
        return !cpu->halted;
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
        return !cpu->halted;
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
    return !cpu->halted;
}

void cpu_run(cpu_t *cpu, uint64_t max_steps)
{
    uint64_t steps = 0;
    while (!cpu->halted) {
        if (max_steps != 0 && steps >= max_steps) {
            break;
        }
        cpu_step(cpu);
        steps++;
    }
}

void cpu_set_trace(cpu_t *cpu, bool enabled, cpu_trace_fn fn, void *user)
{
    cpu->trace_enabled = enabled;
    cpu->trace_fn = fn;
    cpu->trace_user = user;
}

void cpu_set_carry(cpu_t *cpu, bool value)
{
    if (value) {
        cpu->psw |= PSW_CY;
    } else {
        cpu->psw &= (uint8_t)~PSW_CY;
    }
}

bool cpu_get_carry(cpu_t *cpu)
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
