#ifndef MCS51_DECODER_H
#define MCS51_DECODER_H

#include <stdint.h>
#include "cpu.h"

typedef enum {
    MN_INVALID = 0,
    MN_NOP,
    MN_MOV,
    MN_MOV_DIRECT_DIRECT,
    MN_MOV_DPTR_IMM,
    MN_ADD,
    MN_ADDC,
    MN_SUBB,
    MN_ANL,
    MN_ORL,
    MN_XRL,
    MN_DA,
    MN_MUL,
    MN_DIV,
    MN_RR,
    MN_RRC,
    MN_RL,
    MN_RLC,
    MN_SWAP,
    MN_INC,
    MN_DEC,
    MN_JC,
    MN_JNC,
    MN_JZ,
    MN_JNZ,
    MN_JB,
    MN_JBC,
    MN_JNB,
    MN_DJNZ,
    MN_CJNE_A_IMM,
    MN_CJNE_A_DIRECT,
    MN_CJNE_AT_RI_IMM,
    MN_CJNE_RN_IMM,
    MN_JMPA_DPTR,
    MN_SJMP,
    MN_AJMP,
    MN_ACALL,
    MN_LJMP,
    MN_LCALL,
    MN_SETB,
    MN_CLR,
    MN_CPL,
    MN_MOVC_A_DPTR,
    MN_MOVC_A_PC,
    MN_MOVX_A_DPTR,
    MN_MOVX_DPTR_A,
    MN_MOVX_A_AT_RI,
    MN_MOVX_AT_RI_A,
    MN_XCH,
    MN_XCHD,
    MN_RET,
    MN_RETI,
    MN_PUSH,
    MN_POP
} mnemonic_t;

typedef enum {
    AM_NONE = 0,
    AM_A,
    AM_C,
    AM_DPTR,
    AM_DIRECT,
    AM_IMM8,
    AM_RN,
    AM_AT_RI,
    AM_BIT,
    AM_BIT_NOT,
    AM_REL8,
    AM_ADDR11,
    AM_ADDR16
} addr_mode_t;

typedef struct {
    addr_mode_t mode;
    uint16_t arg;
} op_t;

typedef struct {
    mnemonic_t mnemonic;
    addr_mode_t dst_mode;
    addr_mode_t src_mode;
    uint8_t size;
    uint8_t cycles;
} instr_desc_t;

instr_desc_t decode(uint8_t opcode);
const char *opcode_name(uint8_t opcode);
const char *mnemonic_name(mnemonic_t mnemonic);

op_t fetch_operand(cpu_t *cpu, addr_mode_t mode, uint8_t opcode);
uint8_t read_op(cpu_t *cpu, op_t op);
void write_op(cpu_t *cpu, op_t op, uint8_t value);

#endif
