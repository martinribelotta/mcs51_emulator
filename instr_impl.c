#include "instr_impl.h"

void exec_nop(cpu_t *cpu)
{
    (void)cpu;
}

void exec_mov(cpu_t *cpu, op_t dst, op_t src)
{
    write_op(cpu, dst, read_op(cpu, src));
}

void exec_mov_direct_direct(cpu_t *cpu, op_t src, op_t dst)
{
    write_op(cpu, dst, read_op(cpu, src));
}

void exec_mov_dptr_imm(cpu_t *cpu, uint16_t value)
{
    cpu->dptr = value;
}

void exec_add(cpu_t *cpu, op_t src)
{
    uint8_t value = read_op(cpu, src);
    uint8_t acc = cpu->acc;
    uint16_t sum = (uint16_t)acc + (uint16_t)value;
    uint8_t result = (uint8_t)sum;
    cpu->acc = result;
    cpu_set_carry(cpu, sum > 0xFF);
    cpu_set_aux_carry(cpu, ((acc & 0x0F) + (value & 0x0F)) > 0x0F);
    cpu_set_overflow(cpu, ((~(acc ^ value)) & (acc ^ result) & 0x80) != 0);
}

void exec_addc(cpu_t *cpu, op_t src)
{
    uint8_t value = read_op(cpu, src);
    uint8_t carry = cpu_get_carry(cpu) ? 1u : 0u;
    uint8_t acc = cpu->acc;
    uint16_t sum = (uint16_t)acc + (uint16_t)value + (uint16_t)carry;
    uint8_t value_carry = (uint8_t)(value + carry);
    uint8_t result = (uint8_t)sum;
    cpu->acc = result;
    cpu_set_carry(cpu, sum > 0xFF);
    cpu_set_aux_carry(cpu, ((acc & 0x0F) + (value & 0x0F) + carry) > 0x0F);
    cpu_set_overflow(cpu, ((~(acc ^ value_carry)) & (acc ^ result) & 0x80) != 0);
}

void exec_subb(cpu_t *cpu, op_t src)
{
    uint8_t value = read_op(cpu, src);
    uint8_t carry = cpu_get_carry(cpu) ? 1u : 0u;
    uint8_t acc = cpu->acc;
    uint8_t value_carry = (uint8_t)(value + carry);
    uint16_t diff = (uint16_t)acc - (uint16_t)value_carry;
    uint8_t result = (uint8_t)diff;
    cpu->acc = result;
    cpu_set_carry(cpu, acc < value_carry);
    cpu_set_aux_carry(cpu, (acc & 0x0F) < ((value & 0x0F) + carry));
    cpu_set_overflow(cpu, ((acc ^ value) & (acc ^ result) & 0x80) != 0);
}

void exec_anl(cpu_t *cpu, op_t dst, op_t src)
{
    uint8_t value = (uint8_t)(read_op(cpu, dst) & read_op(cpu, src));
    write_op(cpu, dst, value);
}

void exec_orl(cpu_t *cpu, op_t dst, op_t src)
{
    uint8_t value = (uint8_t)(read_op(cpu, dst) | read_op(cpu, src));
    write_op(cpu, dst, value);
}

void exec_xrl(cpu_t *cpu, op_t dst, op_t src)
{
    uint8_t value = (uint8_t)(read_op(cpu, dst) ^ read_op(cpu, src));
    write_op(cpu, dst, value);
}

void exec_da(cpu_t *cpu)
{
    uint8_t acc = cpu->acc;
    uint8_t add = 0;
    bool carry = cpu_get_carry(cpu);
    if (((acc & 0x0F) > 9) || (cpu->psw & 0x40)) {
        add |= 0x06;
    }
    if ((acc > 0x99) || carry) {
        add |= 0x60;
        carry = true;
    }
    cpu->acc = (uint8_t)(acc + add);
    cpu_set_carry(cpu, carry);
}

void exec_mul(cpu_t *cpu)
{
    uint16_t product = (uint16_t)cpu->acc * (uint16_t)cpu->b;
    cpu->acc = (uint8_t)(product & 0xFF);
    cpu->b = (uint8_t)(product >> 8);
    cpu_set_carry(cpu, false);
    cpu_set_overflow(cpu, product > 0xFF);
}

void exec_div(cpu_t *cpu)
{
    uint8_t divisor = cpu->b;
    if (divisor == 0) {
        cpu_set_overflow(cpu, true);
        cpu_set_carry(cpu, false);
        return;
    }
    uint8_t dividend = cpu->acc;
    cpu->acc = (uint8_t)(dividend / divisor);
    cpu->b = (uint8_t)(dividend % divisor);
    cpu_set_overflow(cpu, false);
    cpu_set_carry(cpu, false);
}

void exec_inc(cpu_t *cpu, op_t dst)
{
    if (dst.mode == AM_DPTR) {
        cpu->dptr++;
        return;
    }
    uint8_t value = (uint8_t)(read_op(cpu, dst) + 1u);
    write_op(cpu, dst, value);
}

void exec_dec(cpu_t *cpu, op_t dst)
{
    uint8_t value = (uint8_t)(read_op(cpu, dst) - 1u);
    write_op(cpu, dst, value);
}

static void exec_jump_rel(cpu_t *cpu, op_t rel, bool take)
{
    if (!take) {
        return;
    }
    int8_t offset = (int8_t)rel.arg;
    cpu->pc = (uint16_t)(cpu->pc + offset);
}

void exec_jc(cpu_t *cpu, op_t rel)
{
    exec_jump_rel(cpu, rel, cpu_get_carry(cpu));
}

void exec_jnc(cpu_t *cpu, op_t rel)
{
    exec_jump_rel(cpu, rel, !cpu_get_carry(cpu));
}

void exec_jz(cpu_t *cpu, op_t rel)
{
    exec_jump_rel(cpu, rel, cpu->acc == 0);
}

void exec_jnz(cpu_t *cpu, op_t rel)
{
    exec_jump_rel(cpu, rel, cpu->acc != 0);
}

void exec_jb(cpu_t *cpu, op_t bit, op_t rel)
{
    exec_jump_rel(cpu, rel, read_op(cpu, bit) != 0);
}

void exec_jbc(cpu_t *cpu, op_t bit, op_t rel)
{
    if (read_op(cpu, bit) != 0) {
        write_op(cpu, bit, 0);
        exec_jump_rel(cpu, rel, true);
    }
}

void exec_jnb(cpu_t *cpu, op_t bit, op_t rel)
{
    exec_jump_rel(cpu, rel, read_op(cpu, bit) == 0);
}

void exec_djnz(cpu_t *cpu, op_t dst, op_t rel)
{
    uint8_t value = (uint8_t)(read_op(cpu, dst) - 1u);
    write_op(cpu, dst, value);
    exec_jump_rel(cpu, rel, value != 0);
}

void exec_cjne(cpu_t *cpu, op_t lhs, op_t rhs, op_t rel)
{
    uint8_t left = read_op(cpu, lhs);
    uint8_t right = read_op(cpu, rhs);
    cpu_set_carry(cpu, left < right);
    exec_jump_rel(cpu, rel, left != right);
}

void exec_jmpa_dptr(cpu_t *cpu)
{
    cpu->pc = (uint16_t)(cpu->dptr + cpu->acc);
}

void exec_sjmp(cpu_t *cpu, op_t rel)
{
    int8_t offset = (int8_t)rel.arg;
    cpu->pc = (uint16_t)(cpu->pc + offset);
}

void exec_ajmp(cpu_t *cpu, op_t addr11)
{
    cpu->pc = (uint16_t)((cpu->pc & 0xF800) | (addr11.arg & 0x07FF));
}

void exec_acall(cpu_t *cpu, op_t addr11)
{
    uint16_t ret = cpu->pc;
    cpu->sp++;
    cpu->iram[cpu->sp] = (uint8_t)(ret & 0xFF);
    cpu->sp++;
    cpu->iram[cpu->sp] = (uint8_t)(ret >> 8);
    cpu->pc = (uint16_t)((cpu->pc & 0xF800) | (addr11.arg & 0x07FF));
}

void exec_ljmp(cpu_t *cpu, op_t addr16)
{
    cpu->pc = (uint16_t)addr16.arg;
}

void exec_lcall(cpu_t *cpu, op_t addr16)
{
    uint16_t ret = cpu->pc;
    cpu->sp++;
    cpu->iram[cpu->sp] = (uint8_t)(ret & 0xFF);
    cpu->sp++;
    cpu->iram[cpu->sp] = (uint8_t)(ret >> 8);
    cpu->pc = (uint16_t)addr16.arg;
}

void exec_setb(cpu_t *cpu, op_t bit)
{
    write_op(cpu, bit, 1);
}

void exec_clr(cpu_t *cpu, op_t bit)
{
    if (bit.mode == AM_A) {
        cpu->acc = 0;
        return;
    }
    write_op(cpu, bit, 0);
}

void exec_cpl(cpu_t *cpu, op_t dst)
{
    switch (dst.mode) {
    case AM_A:
        cpu->acc = (uint8_t)~cpu->acc;
        break;
    case AM_C:
        cpu_set_carry(cpu, !cpu_get_carry(cpu));
        break;
    case AM_BIT: {
        uint8_t bit = (uint8_t)dst.arg;
        cpu_write_bit(cpu, bit, cpu_read_bit(cpu, bit) == 0);
        break; }
    default: {
        uint8_t value = read_op(cpu, dst);
        write_op(cpu, dst, (uint8_t)(value ? 0u : 1u));
        break; }
    }
}

void exec_movc_a_dptr(cpu_t *cpu)
{
    uint16_t addr = (uint16_t)(cpu->dptr + cpu->acc);
    cpu->acc = cpu_code_read(cpu, addr);
}

void exec_movc_a_pc(cpu_t *cpu)
{
    uint16_t addr = (uint16_t)(cpu->pc + cpu->acc);
    cpu->acc = cpu_code_read(cpu, addr);
}

void exec_movx_a_dptr(cpu_t *cpu)
{
    cpu->acc = cpu_xdata_read(cpu, cpu->dptr);
}

void exec_movx_dptr_a(cpu_t *cpu)
{
    cpu_xdata_write(cpu, cpu->dptr, cpu->acc);
}

void exec_movx_a_at_ri(cpu_t *cpu, op_t ri)
{
    uint8_t bank = (uint8_t)((cpu->psw >> 3) & 0x03);
    uint8_t addr = cpu->iram[(uint8_t)(bank * 8 + ri.arg)];
    cpu->acc = cpu_xdata_read(cpu, addr);
}

void exec_movx_at_ri_a(cpu_t *cpu, op_t ri)
{
    uint8_t bank = (uint8_t)((cpu->psw >> 3) & 0x03);
    uint8_t addr = cpu->iram[(uint8_t)(bank * 8 + ri.arg)];
    cpu_xdata_write(cpu, addr, cpu->acc);
}

void exec_xch(cpu_t *cpu, op_t src)
{
    uint8_t tmp = cpu->acc;
    cpu->acc = read_op(cpu, src);
    write_op(cpu, src, tmp);
}

void exec_xchd(cpu_t *cpu, op_t ri)
{
    uint8_t bank = (uint8_t)((cpu->psw >> 3) & 0x03);
    uint8_t addr = cpu->iram[(uint8_t)(bank * 8 + ri.arg)];
    uint8_t value = cpu->iram[addr];
    uint8_t acc_low = (uint8_t)(cpu->acc & 0x0F);
    uint8_t mem_low = (uint8_t)(value & 0x0F);
    cpu->acc = (uint8_t)((cpu->acc & 0xF0) | mem_low);
    cpu->iram[addr] = (uint8_t)((value & 0xF0) | acc_low);
}

void exec_ret(cpu_t *cpu)
{
    uint8_t high = cpu->iram[cpu->sp--];
    uint8_t low = cpu->iram[cpu->sp--];
    cpu->pc = (uint16_t)((high << 8) | low);
}

void exec_reti(cpu_t *cpu)
{
    exec_ret(cpu);
}

void exec_push(cpu_t *cpu, op_t direct)
{
    cpu->sp++;
    cpu->iram[cpu->sp] = read_op(cpu, direct);
}

void exec_pop(cpu_t *cpu, op_t direct)
{
    write_op(cpu, direct, cpu->iram[cpu->sp]);
    cpu->sp--;
}
