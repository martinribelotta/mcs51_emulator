#include "test_common.h"

#include <string.h>

int test_failures_count = 0;

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

static const mem_map_region_t code_region = {
    .base = 0x0000,
    .size = 65536u,
    .read = test_code_read,
    .write = test_code_write,
    .user = mem.code,
};

static const mem_map_region_t xdata_region = {
    .base = 0x0000,
    .size = 65536u,
    .read = test_xdata_read,
    .write = test_xdata_write,
    .user = mem.xdata,
};

static const mem_map_t const_map = {
    .code_regions = &code_region,
    .code_region_count = 1,
    .xdata_regions = &xdata_region,
    .xdata_region_count = 1,
};

void test_reset_failures(void)
{
    test_failures_count = 0;
}

int test_failures(void)
{
    return test_failures_count;
}

void load_code(cpu_t *cpu, const uint8_t *code, size_t len)
{
    cpu_init(cpu);
    mem_map_attach(cpu, &const_map);
    memset(mem.code, 0xFF, sizeof(mem.code));
    memset(mem.xdata, 0x00, sizeof(mem.xdata));
    memcpy(mem.code, code, len);
    cpu->pc = 0;
}

test_mem_t *get_mem(cpu_t *cpu)
{
    (void)cpu;
    return &mem;
}
