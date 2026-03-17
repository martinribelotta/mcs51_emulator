#include <stdio.h>
#include <string.h>
#include "cpu.h"
#include "mem_map.h"

static int failures = 0;

typedef struct {
    uint8_t code[65536];
    uint8_t xdata[65536];
    mem_map_t map;
    mem_map_region_t code_region;
    mem_map_region_t xdata_region;
} test_mem_t;

static test_mem_t mem;

static uint8_t test_code_read(const cpu_t *cpu, uint16_t addr, void *user)
{
    (void)cpu;
    uint8_t *code = (uint8_t *)user;
    return code[addr];
}

static void test_code_write(cpu_t *cpu, uint16_t addr, uint8_t value, void *user)
{
    (void)cpu;
    uint8_t *code = (uint8_t *)user;
    code[addr] = value;
}

static uint8_t test_xdata_read(const cpu_t *cpu, uint16_t addr, void *user)
{
    (void)cpu;
    uint8_t *xdata = (uint8_t *)user;
    return xdata[addr];
}

static void test_xdata_write(cpu_t *cpu, uint16_t addr, uint8_t value, void *user)
{
    (void)cpu;
    uint8_t *xdata = (uint8_t *)user;
    xdata[addr] = value;
}

#define ASSERT_EQ(msg, a, b) do { \
    if ((a) != (b)) { \
        printf("FAIL: %s (got 0x%X, expected 0x%X)\n", msg, (unsigned)(a), (unsigned)(b)); \
        failures++; \
    } \
} while (0)

#define ASSERT_TRUE(msg, cond) do { \
    if (!(cond)) { \
        printf("FAIL: %s\n", msg); \
        failures++; \
    } \
} while (0)

static void load_code(cpu_t *cpu, const uint8_t *code, size_t len)
{
    cpu_init(cpu);
    mem_map_init(&mem.map);
    mem.code_region.base = 0x0000;
    mem.code_region.size = 65536u;
    mem.code_region.read = test_code_read;
    mem.code_region.write = test_code_write;
    mem.code_region.user = mem.code;
    mem.xdata_region.base = 0x0000;
    mem.xdata_region.size = 65536u;
    mem.xdata_region.read = test_xdata_read;
    mem.xdata_region.write = test_xdata_write;
    mem.xdata_region.user = mem.xdata;
    mem_map_set_code_regions(&mem.map, &mem.code_region, 1);
    mem_map_set_xdata_regions(&mem.map, &mem.xdata_region, 1);
    mem_map_attach(cpu, &mem.map);
    memset(mem.code, 0xFF, sizeof(mem.code));
    memset(mem.xdata, 0x00, sizeof(mem.xdata));
    memcpy(mem.code, code, len);
    cpu->pc = 0;
}

static test_mem_t *get_mem(cpu_t *cpu)
{
    (void)cpu;
    return &mem;
}

static void test_nop(void)
{
    cpu_t cpu;
    uint8_t code[] = { 0x00 };
    load_code(&cpu, code, sizeof(code));
    cpu_step(&cpu);
    ASSERT_EQ("NOP pc", cpu.pc, 1);
}

static void test_inc_dec_a(void)
{
    cpu_t cpu;
    uint8_t code[] = { 0x04, 0x14 };
    load_code(&cpu, code, sizeof(code));
    cpu.acc = 0x0F;
    cpu_step(&cpu);
    ASSERT_EQ("INC A", cpu.acc, 0x10);
    cpu_step(&cpu);
    ASSERT_EQ("DEC A", cpu.acc, 0x0F);
}

static void test_add_addc_subb(void)
{
    cpu_t cpu;
    uint8_t code[] = { 0x24, 0x02, 0x34, 0x00, 0x94, 0x01 };
    load_code(&cpu, code, sizeof(code));
    cpu.acc = 0xFE;
    cpu_step(&cpu);
    ASSERT_EQ("ADD A,#imm", cpu.acc, 0x00);
    ASSERT_TRUE("ADD carry", cpu_get_carry(&cpu));
    cpu_set_carry(&cpu, true);
    cpu_step(&cpu);
    ASSERT_EQ("ADDC A,#imm", cpu.acc, 0x01);
    cpu_set_carry(&cpu, false);
    cpu_step(&cpu);
    ASSERT_EQ("SUBB A,#imm", cpu.acc, 0x00);
}

static void test_mov_basic(void)
{
    cpu_t cpu;
    uint8_t code[] = {
        0x74, 0x12,
        0x75, 0x30, 0x34,
        0xE5, 0x30,
        0xF5, 0x31,
        0x85, 0x30, 0x35,
        0x90, 0x12, 0x34,
        0xA2, 0x00,
        0x92, 0x01
    };
    load_code(&cpu, code, sizeof(code));
    cpu.iram[0x30] = 0x34;
    cpu.iram[0x20] = 0x01;

    cpu_step(&cpu);
    ASSERT_EQ("MOV A,#imm", cpu.acc, 0x12);
    cpu_step(&cpu);
    ASSERT_EQ("MOV direct,#imm", cpu.iram[0x30], 0x34);
    cpu_step(&cpu);
    ASSERT_EQ("MOV A,direct", cpu.acc, 0x34);
    cpu_step(&cpu);
    ASSERT_EQ("MOV direct,A", cpu.iram[0x31], 0x34);
    cpu.iram[0x30] = 0x5A;
    cpu_step(&cpu);
    ASSERT_EQ("MOV direct,direct", cpu.iram[0x35], 0x5A);
    cpu_step(&cpu);
    ASSERT_EQ("MOV DPTR,#imm16", cpu.dptr, 0x1234);
    cpu_step(&cpu);
    ASSERT_TRUE("MOV C,bit", cpu_get_carry(&cpu));
    cpu_set_carry(&cpu, true);
    cpu_step(&cpu);
    ASSERT_EQ("MOV bit,C", cpu.iram[0x20] & 0x02, 0x02);
}

static void test_mov_indirect_and_rn(void)
{
    cpu_t cpu;
    uint8_t code[] = {
        0x76, 0x56,
        0xE6,
        0xF6,
        0x86, 0x33,
        0xA6, 0x34,
        0x78, 0x78,
        0xE8,
        0xF8,
        0x88, 0x32,
        0xA8, 0x32
    };
    load_code(&cpu, code, sizeof(code));

    cpu.iram[0] = 0x40;
    cpu_step(&cpu);
    ASSERT_EQ("MOV @Ri,#imm", cpu.iram[0x40], 0x56);

    cpu.iram[0x40] = 0x9A;
    cpu.iram[0] = 0x40;
    cpu_step(&cpu);
    ASSERT_EQ("MOV A,@Ri", cpu.acc, 0x9A);

    cpu.acc = 0x66;
    cpu.iram[0] = 0x40;
    cpu_step(&cpu);
    ASSERT_EQ("MOV @Ri,A", cpu.iram[0x40], 0x66);

    cpu.iram[0x40] = 0x99;
    cpu.iram[0] = 0x40;
    cpu_step(&cpu);
    ASSERT_EQ("MOV direct,@Ri", cpu.iram[0x33], 0x99);

    cpu.iram[0x34] = 0xAA;
    cpu.iram[0] = 0x40;
    cpu_step(&cpu);
    ASSERT_EQ("MOV @Ri,direct", cpu.iram[0x40], 0xAA);

    cpu_step(&cpu);
    ASSERT_EQ("MOV Rn,#imm", cpu.iram[0], 0x78);

    cpu.iram[0] = 0x55;
    cpu_step(&cpu);
    ASSERT_EQ("MOV A,Rn", cpu.acc, 0x55);

    cpu.acc = 0x77;
    cpu_step(&cpu);
    ASSERT_EQ("MOV Rn,A", cpu.iram[0], 0x77);

    cpu_step(&cpu);
    ASSERT_EQ("MOV direct,Rn", cpu.iram[0x32], 0x77);

    cpu.iram[0x32] = 0x88;
    cpu_step(&cpu);
    ASSERT_EQ("MOV Rn,direct", cpu.iram[0], 0x88);
}

static void test_logic_ops(void)
{
    cpu_t cpu;
    uint8_t code[] = {
        0x54, 0x0F,
        0x42, 0x30,
        0x43, 0x31, 0xF0,
        0x44, 0x01,
        0x62, 0x32,
        0x63, 0x33, 0x0F,
        0x82, 0x00,
        0xA0, 0x01
    };
    load_code(&cpu, code, sizeof(code));
    cpu.acc = 0xF3;
    cpu.iram[0x30] = 0x0F;
    cpu.iram[0x31] = 0x0F;
    cpu.iram[0x32] = 0xF0;
    cpu.iram[0x33] = 0xF0;
    cpu.iram[0x20] = 0x01;

    cpu_step(&cpu);
    ASSERT_EQ("ANL A,#imm", cpu.acc, 0x03);
    cpu.acc = 0xF0;
    cpu_step(&cpu);
    ASSERT_EQ("ORL direct,A", cpu.iram[0x30], 0xFF);
    cpu_step(&cpu);
    ASSERT_EQ("ORL direct,#imm", cpu.iram[0x31], 0xFF);
    cpu.acc = 0x00;
    cpu_step(&cpu);
    ASSERT_EQ("ORL A,#imm", cpu.acc, 0x01);
    cpu.acc = 0xFF;
    cpu_step(&cpu);
    ASSERT_EQ("XRL direct,A", cpu.iram[0x32], 0x0F);
    cpu_step(&cpu);
    ASSERT_EQ("XRL direct,#imm", cpu.iram[0x33], 0xFF);
    cpu_set_carry(&cpu, true);
    cpu_step(&cpu);
    ASSERT_TRUE("ANL C,bit", cpu_get_carry(&cpu));
    cpu_step(&cpu);
    ASSERT_TRUE("ORL C,/bit", cpu_get_carry(&cpu));
}

static void test_jumps_basic(void)
{
    cpu_t cpu;

    uint8_t code_jc[] = { 0x40, 0x02 };
    load_code(&cpu, code_jc, sizeof(code_jc));
    cpu_set_carry(&cpu, true);
    cpu_step(&cpu);
    ASSERT_EQ("JC taken", cpu.pc, 4);

    uint8_t code_jnc[] = { 0x50, 0x02 };
    load_code(&cpu, code_jnc, sizeof(code_jnc));
    cpu_set_carry(&cpu, false);
    cpu_step(&cpu);
    ASSERT_EQ("JNC taken", cpu.pc, 4);

    uint8_t code_jz[] = { 0x60, 0x02 };
    load_code(&cpu, code_jz, sizeof(code_jz));
    cpu.acc = 0;
    cpu_step(&cpu);
    ASSERT_EQ("JZ taken", cpu.pc, 4);

    uint8_t code_jnz[] = { 0x70, 0x02 };
    load_code(&cpu, code_jnz, sizeof(code_jnz));
    cpu.acc = 1;
    cpu_step(&cpu);
    ASSERT_EQ("JNZ taken", cpu.pc, 4);
}

static void test_bit_jumps(void)
{
    cpu_t cpu;
    uint8_t code_jb[] = { 0x20, 0x00, 0x02 };
    load_code(&cpu, code_jb, sizeof(code_jb));
    cpu.iram[0x20] = 0x01;
    cpu_step(&cpu);
    ASSERT_EQ("JB taken", cpu.pc, 5);

    uint8_t code_jbc[] = { 0x10, 0x00, 0x02 };
    load_code(&cpu, code_jbc, sizeof(code_jbc));
    cpu.iram[0x20] = 0x01;
    cpu_step(&cpu);
    ASSERT_EQ("JBC taken", cpu.pc, 5);
    ASSERT_EQ("JBC cleared", cpu.iram[0x20] & 0x01, 0x00);

    uint8_t code_jnb[] = { 0x30, 0x00, 0x02 };
    load_code(&cpu, code_jnb, sizeof(code_jnb));
    cpu.iram[0x20] = 0x00;
    cpu_step(&cpu);
    ASSERT_EQ("JNB taken", cpu.pc, 5);
}

static void test_djnz_cjne(void)
{
    cpu_t cpu;
    uint8_t code_djnz_d[] = { 0xD5, 0x30, 0x02 };
    load_code(&cpu, code_djnz_d, sizeof(code_djnz_d));
    cpu.iram[0x30] = 2;
    cpu_step(&cpu);
    ASSERT_EQ("DJNZ direct", cpu.pc, 5);

    uint8_t code_djnz_r[] = { 0xD8, 0x02 };
    load_code(&cpu, code_djnz_r, sizeof(code_djnz_r));
    cpu.iram[0] = 2;
    cpu_step(&cpu);
    ASSERT_EQ("DJNZ Rn", cpu.pc, 4);

    uint8_t code_cjne_a_imm[] = { 0xB4, 0x01, 0x02 };
    load_code(&cpu, code_cjne_a_imm, sizeof(code_cjne_a_imm));
    cpu.acc = 0;
    cpu_step(&cpu);
    ASSERT_EQ("CJNE A,#imm", cpu.pc, 5);

    uint8_t code_cjne_a_dir[] = { 0xB5, 0x31, 0x02 };
    load_code(&cpu, code_cjne_a_dir, sizeof(code_cjne_a_dir));
    cpu.acc = 2;
    cpu.iram[0x31] = 3;
    cpu_step(&cpu);
    ASSERT_EQ("CJNE A,direct", cpu.pc, 5);

    uint8_t code_cjne_ri[] = { 0xB6, 0x02, 0x02 };
    load_code(&cpu, code_cjne_ri, sizeof(code_cjne_ri));
    cpu.iram[0] = 0x40;
    cpu.iram[0x40] = 1;
    cpu_step(&cpu);
    ASSERT_EQ("CJNE @Ri,#imm", cpu.pc, 5);

    uint8_t code_cjne_rn[] = { 0xB8, 0x02, 0x02 };
    load_code(&cpu, code_cjne_rn, sizeof(code_cjne_rn));
    cpu.iram[0] = 1;
    cpu_step(&cpu);
    ASSERT_EQ("CJNE Rn,#imm", cpu.pc, 5);
}

static void test_calls_jmp(void)
{
    cpu_t cpu;
    uint8_t code_acall[] = { 0x11, 0x03, 0x00, 0x22 };
    load_code(&cpu, code_acall, sizeof(code_acall));
    cpu_step(&cpu);
    ASSERT_EQ("ACALL", cpu.pc, 0x0003);
    cpu_step(&cpu);
    ASSERT_EQ("RET", cpu.pc, 0x0002);

    uint8_t code_lcall[] = { 0x12, 0x00, 0x05, 0x00, 0x00, 0x22 };
    load_code(&cpu, code_lcall, sizeof(code_lcall));
    cpu_step(&cpu);
    ASSERT_EQ("LCALL", cpu.pc, 0x0005);
    cpu_step(&cpu);
    ASSERT_EQ("RET after LCALL", cpu.pc, 0x0003);

    uint8_t code_jmp[] = { 0x73 };
    load_code(&cpu, code_jmp, sizeof(code_jmp));
    cpu.acc = 0x02;
    cpu.dptr = 0x0010;
    cpu_step(&cpu);
    ASSERT_EQ("JMP @A+DPTR", cpu.pc, 0x0012);
}

static void test_movc_movx(void)
{
    cpu_t cpu;
    uint8_t code[] = { 0x93, 0xE0, 0xF0, 0xE2, 0xF2 };
    load_code(&cpu, code, sizeof(code));
    test_mem_t *mem = get_mem(&cpu);
    cpu.dptr = 0x0100;
    cpu.acc = 0x02;
    mem->code[0x0102] = 0xAA;
    cpu_step(&cpu);
    ASSERT_EQ("MOVC A,@A+DPTR", cpu.acc, 0xAA);

    cpu.dptr = 0x0200;
    mem->xdata[0x0200] = 0xCC;
    cpu_step(&cpu);
    ASSERT_EQ("MOVX A,@DPTR", cpu.acc, 0xCC);

    cpu.acc = 0xDD;
    cpu.dptr = 0x0200;
    cpu_step(&cpu);
    ASSERT_EQ("MOVX @DPTR,A", mem->xdata[0x0200], 0xDD);

    cpu.iram[0] = 0x10;
    mem->xdata[0x10] = 0xEE;
    cpu_step(&cpu);
    ASSERT_EQ("MOVX A,@Ri", cpu.acc, 0xEE);

    cpu.acc = 0x99;
    cpu.iram[0] = 0x10;
    cpu_step(&cpu);
    ASSERT_EQ("MOVX @Ri,A", mem->xdata[0x10], 0x99);
}

static void test_movc_pc(void)
{
    cpu_t cpu;
    uint8_t code[] = { 0x83, 0x00, 0x55 };
    load_code(&cpu, code, sizeof(code));
    cpu.acc = 0x01;
    cpu_step(&cpu);
    ASSERT_EQ("MOVC A,@A+PC", cpu.acc, 0x55);
}

static void test_da_mul_div_xch(void)
{
    cpu_t cpu;
    uint8_t code[] = { 0xD4, 0xA4, 0x84, 0xC5, 0x30, 0xC6, 0xC8, 0xD6 };
    load_code(&cpu, code, sizeof(code));

    cpu.acc = 0x9B;
    cpu.psw |= 0x40;
    cpu_step(&cpu);
    ASSERT_EQ("DA A", cpu.acc, 0x01);
    ASSERT_TRUE("DA carry", cpu_get_carry(&cpu));

    cpu.acc = 0x10;
    cpu.b = 0x10;
    cpu_step(&cpu);
    ASSERT_EQ("MUL AB acc", cpu.acc, 0x00);
    ASSERT_EQ("MUL AB b", cpu.b, 0x01);

    cpu.acc = 0x10;
    cpu.b = 0x04;
    cpu_step(&cpu);
    ASSERT_EQ("DIV AB acc", cpu.acc, 0x04);
    ASSERT_EQ("DIV AB b", cpu.b, 0x00);

    cpu.acc = 0xAA;
    cpu.iram[0x30] = 0x55;
    cpu_step(&cpu);
    ASSERT_EQ("XCH A,direct acc", cpu.acc, 0x55);
    ASSERT_EQ("XCH A,direct mem", cpu.iram[0x30], 0xAA);

    cpu.iram[0] = 0x40;
    cpu.iram[0x40] = 0x12;
    cpu.acc = 0x34;
    cpu_step(&cpu);
    ASSERT_EQ("XCH A,@Ri", cpu.acc, 0x12);

    cpu.iram[0] = 0x56;
    cpu.acc = 0x78;
    cpu_step(&cpu);
    ASSERT_EQ("XCH A,Rn", cpu.acc, 0x56);

    cpu.iram[0] = 0x41;
    cpu.iram[0x41] = 0xAB;
    cpu.acc = 0xCD;
    cpu_step(&cpu);
    ASSERT_EQ("XCHD A,@Ri", cpu.acc, 0xCB);
    ASSERT_EQ("XCHD A,@Ri mem", cpu.iram[0x41], 0xAD);
}

static void test_inc_dec_misc(void)
{
    cpu_t cpu;
    uint8_t code[] = { 0x05, 0x30, 0x15, 0x30, 0xA3, 0xE4 };
    load_code(&cpu, code, sizeof(code));
    cpu.iram[0x30] = 0x01;
    cpu_step(&cpu);
    ASSERT_EQ("INC direct", cpu.iram[0x30], 0x02);
    cpu_step(&cpu);
    ASSERT_EQ("DEC direct", cpu.iram[0x30], 0x01);
    cpu.dptr = 0x1234;
    cpu_step(&cpu);
    ASSERT_EQ("INC DPTR", cpu.dptr, 0x1235);
    cpu.acc = 0xFF;
    cpu_step(&cpu);
    ASSERT_EQ("CLR A", cpu.acc, 0x00);
}

static uint8_t parity_bit(uint8_t acc)
{
    uint8_t v = acc;
    v ^= (uint8_t)(v >> 4);
    v ^= (uint8_t)(v >> 2);
    v ^= (uint8_t)(v >> 1);
    return (uint8_t)(v & 1u);
}

static void test_parity_flag(void)
{
    cpu_t cpu;
    uint8_t code[] = { 0x74, 0x00, 0x74, 0x01, 0x74, 0xFF };
    load_code(&cpu, code, sizeof(code));

    cpu_step(&cpu);
    ASSERT_EQ("P for A=00", cpu.psw & 0x01, parity_bit(cpu.acc));

    cpu_step(&cpu);
    ASSERT_EQ("P for A=01", cpu.psw & 0x01, parity_bit(cpu.acc));

    cpu_step(&cpu);
    ASSERT_EQ("P for A=FF", cpu.psw & 0x01, parity_bit(cpu.acc));
}

static void test_add_flags(void)
{
    cpu_t cpu;
    uint8_t code[] = { 0x24, 0x01, 0x24, 0x01, 0x24, 0x01 };
    load_code(&cpu, code, sizeof(code));

    cpu.acc = 0x0F;
    cpu_step(&cpu);
    ASSERT_EQ("ADD AC", cpu.psw & 0x40, 0x40);
    ASSERT_EQ("ADD CY", cpu.psw & 0x80, 0x00);
    ASSERT_EQ("ADD OV", cpu.psw & 0x04, 0x00);

    cpu.acc = 0x7F;
    cpu_step(&cpu);
    ASSERT_EQ("ADD OV", cpu.psw & 0x04, 0x04);

    cpu.acc = 0xFF;
    cpu_step(&cpu);
    ASSERT_EQ("ADD CY", cpu.psw & 0x80, 0x80);
}

static void test_addc_subb_flags(void)
{
    cpu_t cpu;
    uint8_t code[] = { 0x34, 0x00, 0x94, 0x00 };
    load_code(&cpu, code, sizeof(code));

    cpu.acc = 0x0F;
    cpu_set_carry(&cpu, true);
    cpu_step(&cpu);
    ASSERT_EQ("ADDC AC", cpu.psw & 0x40, 0x40);

    cpu.acc = 0x00;
    cpu_set_carry(&cpu, true);
    cpu_step(&cpu);
    ASSERT_EQ("SUBB CY", cpu.psw & 0x80, 0x80);
}

static void test_da_flags(void)
{
    cpu_t cpu;
    uint8_t code[] = { 0xD4 };
    load_code(&cpu, code, sizeof(code));
    cpu.acc = 0x9B;
    cpu.psw |= 0x40;
    cpu_step(&cpu);
    ASSERT_EQ("DA carry", cpu.psw & 0x80, 0x80);
}

int main(void)
{
    test_nop();
    test_inc_dec_a();
    test_add_addc_subb();
    test_mov_basic();
    test_mov_indirect_and_rn();
    test_logic_ops();
    test_jumps_basic();
    test_bit_jumps();
    test_djnz_cjne();
    test_calls_jmp();
    test_movc_movx();
    test_movc_pc();
    test_da_mul_div_xch();
    test_inc_dec_misc();
    test_parity_flag();
    test_add_flags();
    test_addc_subb_flags();
    test_da_flags();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    printf("%d tests failed.\n", failures);
    return 1;
}
