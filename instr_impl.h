#ifndef MCS51_INSTR_IMPL_H
#define MCS51_INSTR_IMPL_H

#include "decoder.h"

void exec_nop(cpu_t *cpu);
void exec_mov(cpu_t *cpu, op_t dst, op_t src);
void exec_mov_direct_direct(cpu_t *cpu, op_t src, op_t dst);
void exec_mov_dptr_imm(cpu_t *cpu, uint16_t value);
void exec_add(cpu_t *cpu, op_t src);
void exec_addc(cpu_t *cpu, op_t src);
void exec_subb(cpu_t *cpu, op_t src);
void exec_anl(cpu_t *cpu, op_t dst, op_t src);
void exec_orl(cpu_t *cpu, op_t dst, op_t src);
void exec_xrl(cpu_t *cpu, op_t dst, op_t src);
void exec_da(cpu_t *cpu);
void exec_mul(cpu_t *cpu);
void exec_div(cpu_t *cpu);
void exec_inc(cpu_t *cpu, op_t dst);
void exec_dec(cpu_t *cpu, op_t dst);
void exec_jc(cpu_t *cpu, op_t rel);
void exec_jnc(cpu_t *cpu, op_t rel);
void exec_jz(cpu_t *cpu, op_t rel);
void exec_jnz(cpu_t *cpu, op_t rel);
void exec_jb(cpu_t *cpu, op_t bit, op_t rel);
void exec_jbc(cpu_t *cpu, op_t bit, op_t rel);
void exec_jnb(cpu_t *cpu, op_t bit, op_t rel);
void exec_djnz(cpu_t *cpu, op_t dst, op_t rel);
void exec_cjne(cpu_t *cpu, op_t lhs, op_t rhs, op_t rel);
void exec_jmpa_dptr(cpu_t *cpu);
void exec_sjmp(cpu_t *cpu, op_t rel);
void exec_ajmp(cpu_t *cpu, op_t addr11);
void exec_acall(cpu_t *cpu, op_t addr11);
void exec_ljmp(cpu_t *cpu, op_t addr16);
void exec_lcall(cpu_t *cpu, op_t addr16);
void exec_setb(cpu_t *cpu, op_t bit);
void exec_clr(cpu_t *cpu, op_t bit);
void exec_cpl(cpu_t *cpu, op_t dst);
void exec_movc_a_dptr(cpu_t *cpu);
void exec_movc_a_pc(cpu_t *cpu);
void exec_movx_a_dptr(cpu_t *cpu);
void exec_movx_dptr_a(cpu_t *cpu);
void exec_movx_a_at_ri(cpu_t *cpu, op_t ri);
void exec_movx_at_ri_a(cpu_t *cpu, op_t ri);
void exec_xch(cpu_t *cpu, op_t src);
void exec_xchd(cpu_t *cpu, op_t ri);
void exec_ret(cpu_t *cpu);
void exec_reti(cpu_t *cpu);
void exec_push(cpu_t *cpu, op_t direct);
void exec_pop(cpu_t *cpu, op_t direct);

#endif
