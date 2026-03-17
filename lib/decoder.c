#include "decoder.h"
#include "instr_list.h"
#include <stddef.h>

static uint8_t reg_bank_base(cpu_t *cpu)
{
    uint8_t bank = (uint8_t)((cpu->psw >> 3) & 0x03);
    return (uint8_t)(bank * 8);
}

static const instr_desc_t k_instr_table[256] = {
#define X(op, mn, dst, src, size, name) [op] = { mn, dst, src, size, 1 },
    INSTR_LIST(X)
#undef X
};

static uint8_t opcode_cycles(uint8_t opcode)
{
    switch (opcode) {
    case 0x01: case 0x02: case 0x10: case 0x11: case 0x12:
    case 0x20: case 0x21: case 0x22: case 0x30: case 0x31: case 0x32:
    case 0x40: case 0x41: case 0x43: case 0x50: case 0x51: case 0x53:
    case 0x60: case 0x61: case 0x63: case 0x70: case 0x71: case 0x72:
    case 0x73: case 0x75: case 0x80: case 0x81: case 0x82: case 0x83:
    case 0x85: case 0x86: case 0x87: case 0x88: case 0x89: case 0x8A:
    case 0x8B: case 0x8C: case 0x8D: case 0x8E: case 0x8F: case 0x90:
    case 0x91: case 0x92: case 0x93: case 0xA0: case 0xA1: case 0xA3:
    case 0xA6: case 0xA7: case 0xA8: case 0xA9: case 0xAA: case 0xAB:
    case 0xAC: case 0xAD: case 0xAE: case 0xAF: case 0xB0: case 0xB1:
    case 0xB4: case 0xB5: case 0xB6: case 0xB7: case 0xB8: case 0xB9:
    case 0xBA: case 0xBB: case 0xBC: case 0xBD: case 0xBE: case 0xBF:
    case 0xC0: case 0xC1: case 0xD0: case 0xD1: case 0xD5: case 0xD8:
    case 0xD9: case 0xDA: case 0xDB: case 0xDC: case 0xDD: case 0xDE:
    case 0xDF: case 0xE0: case 0xE1: case 0xE2: case 0xE3: case 0xF0:
    case 0xF1: case 0xF2: case 0xF3:
        return 2;
    case 0x84: case 0xA4:
        return 4;
    default:
        return 1;
    }
}

static const char *const k_opcode_names[256] = {
#define X(op, mn, dst, src, size, name) [op] = name,
    INSTR_LIST(X)
#undef X
};

static const char *const k_mnemonic_names[] = {
    "INVALID",
    "NOP",
    "MOV",
    "MOV_DIRECT_DIRECT",
    "MOV_DPTR_IMM",
    "ADD",
    "ADDC",
    "SUBB",
    "ANL",
    "ORL",
    "XRL",
    "DA",
    "MUL",
    "DIV",
    "RR",
    "RRC",
    "RL",
    "RLC",
    "SWAP",
    "INC",
    "DEC",
    "JC",
    "JNC",
    "JZ",
    "JNZ",
    "JB",
    "JBC",
    "JNB",
    "DJNZ",
    "CJNE_A_IMM",
    "CJNE_A_DIRECT",
    "CJNE_AT_RI_IMM",
    "CJNE_RN_IMM",
    "JMPA_DPTR",
    "SJMP",
    "AJMP",
    "ACALL",
    "LJMP",
    "LCALL",
    "SETB",
    "CLR",
    "CPL",
    "MOVC_A_DPTR",
    "MOVC_A_PC",
    "MOVX_A_DPTR",
    "MOVX_DPTR_A",
    "MOVX_A_AT_RI",
    "MOVX_AT_RI_A",
    "XCH",
    "XCHD",
    "RET",
    "RETI",
    "PUSH",
    "POP"
};

instr_desc_t decode(uint8_t opcode)
{
    instr_desc_t desc = k_instr_table[opcode];
    if (desc.mnemonic == MN_INVALID) {
        desc.size = 1;
        desc.dst_mode = AM_NONE;
        desc.src_mode = AM_NONE;
        desc.cycles = 1;
    } else {
        desc.cycles = opcode_cycles(opcode);
    }
    return desc;
}

const char *opcode_name(uint8_t opcode)
{
    const char *name = k_opcode_names[opcode];
    return name ? name : "???";
}

const char *mnemonic_name(mnemonic_t mnemonic)
{
    if (mnemonic < 0 || (size_t)mnemonic >= MCS51_ARRAY_LEN(k_mnemonic_names)) {
        return "INVALID";
    }
    return k_mnemonic_names[mnemonic];
}

op_t fetch_operand(cpu_t *cpu, addr_mode_t mode, uint8_t opcode)
{
    op_t op = { mode, 0 };
    switch (mode) {
    case AM_IMM8:
        op.arg = cpu_fetch8(cpu);
        break;
    case AM_DIRECT:
        op.arg = cpu_fetch8(cpu);
        break;
    case AM_BIT:
        op.arg = cpu_fetch8(cpu);
        break;
    case AM_REL8:
        op.arg = cpu_fetch8(cpu);
        break;
    case AM_ADDR16:
        op.arg = cpu_fetch16(cpu);
        break;
    case AM_ADDR11: {
        uint8_t low = cpu_fetch8(cpu);
        op.arg = (uint16_t)(((opcode & 0xE0) << 3) | low);
        break; }
    case AM_C:
        op.arg = 0;
        break;
    case AM_DPTR:
        op.arg = 0;
        break;
    case AM_RN:
        op.arg = (uint16_t)(opcode & 0x07);
        break;
    case AM_AT_RI:
        op.arg = (uint16_t)(opcode & 0x01);
        break;
    default:
        break;
    }
    return op;
}

uint8_t read_op(cpu_t *cpu, op_t op)
{
    switch (op.mode) {
    case AM_A:
        return cpu->acc;
    case AM_C:
        return (uint8_t)(cpu_get_carry(cpu) ? 1u : 0u);
    case AM_IMM8:
        return (uint8_t)op.arg;
    case AM_DIRECT:
        return cpu_read_direct(cpu, (uint8_t)op.arg);
    case AM_RN: {
        uint8_t base = reg_bank_base(cpu);
        return cpu->iram[(uint8_t)(base + op.arg)]; }
    case AM_AT_RI: {
        uint8_t base = reg_bank_base(cpu);
        uint8_t addr = cpu->iram[(uint8_t)(base + op.arg)];
        return cpu->iram[addr]; }
    case AM_BIT:
        return cpu_read_bit(cpu, (uint8_t)op.arg);
    case AM_BIT_NOT:
        return (uint8_t)(cpu_read_bit(cpu, (uint8_t)op.arg) ? 0u : 1u);
    default:
        return 0;
    }
}

void write_op(cpu_t *cpu, op_t op, uint8_t value)
{
    switch (op.mode) {
    case AM_A:
        cpu->acc = value;
        break;
    case AM_C:
        cpu_set_carry(cpu, value != 0);
        break;
    case AM_DIRECT:
        cpu_write_direct(cpu, (uint8_t)op.arg, value);
        break;
    case AM_RN: {
        uint8_t base = reg_bank_base(cpu);
        cpu->iram[(uint8_t)(base + op.arg)] = value;
        break; }
    case AM_AT_RI: {
        uint8_t base = reg_bank_base(cpu);
        uint8_t addr = cpu->iram[(uint8_t)(base + op.arg)];
        cpu->iram[addr] = value;
        break; }
    case AM_BIT:
        cpu_write_bit(cpu, (uint8_t)op.arg, value != 0);
        break;
    case AM_BIT_NOT:
        cpu_write_bit(cpu, (uint8_t)op.arg, value == 0);
        break;
    default:
        break;
    }
}
