#include <stdio.h>

#include "test_common.h"

#ifndef TEST_ENTRY
#error "TEST_ENTRY must be defined"
#endif

#define STR2(x) #x
#define STR(x) STR2(x)

void TEST_ENTRY(void);

int main(void)
{
    test_reset_failures();
    TEST_ENTRY();

    if (test_failures() == 0) {
        printf("Test %s passed.\n", STR(TEST_ENTRY));
        return 0;
    }

    printf("Test %s failed with %d assertion(s).\n", STR(TEST_ENTRY), test_failures());
    return 1;
}
