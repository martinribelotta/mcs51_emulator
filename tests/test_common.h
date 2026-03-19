#ifndef MCS51_TEST_COMMON_H
#define MCS51_TEST_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "cpu.h"
#include "mem_map.h"

typedef struct {
    uint8_t code[65536];
    uint8_t xdata[65536];
} test_mem_t;

extern int test_failures_count;

#define ASSERT_EQ(msg, a, b) do { \
    if ((a) != (b)) { \
        printf("FAIL: %s (got 0x%X, expected 0x%X)\n", msg, (unsigned)(a), (unsigned)(b)); \
        test_failures_count++; \
    } \
} while (0)

#define ASSERT_TRUE(msg, cond) do { \
    if (!(cond)) { \
        printf("FAIL: %s\n", msg); \
        test_failures_count++; \
    } \
} while (0)

void test_reset_failures(void);
int test_failures(void);

void load_code(cpu_t *cpu, const uint8_t *code, size_t len);
test_mem_t *get_mem(cpu_t *cpu);

#endif
