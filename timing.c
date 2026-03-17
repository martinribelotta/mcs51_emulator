#include "timing.h"

static uint64_t mul_div_u64(uint64_t a, uint64_t b, uint64_t c)
{
#if defined(__SIZEOF_INT128__)
    __uint128_t prod = (__uint128_t)a * (__uint128_t)b;
    return (uint64_t)(prod / c);
#else
    double prod = (double)a * (double)b;
    return (uint64_t)(prod / (double)c);
#endif
}

void timing_init(timing_t *t, uint32_t fosc_hz, uint8_t clocks_per_cycle)
{
    if (!t) {
        return;
    }
    t->fosc_hz = fosc_hz;
    t->clocks_per_cycle = clocks_per_cycle ? clocks_per_cycle : 12;
    t->cycles_total = 0;
}

void timing_reset(timing_t *t)
{
    if (!t) {
        return;
    }
    t->cycles_total = 0;
}

void timing_set_clock(timing_t *t, uint32_t fosc_hz, uint8_t clocks_per_cycle)
{
    if (!t) {
        return;
    }
    t->fosc_hz = fosc_hz;
    t->clocks_per_cycle = clocks_per_cycle ? clocks_per_cycle : t->clocks_per_cycle;
}

uint64_t timing_cycles_per_second(const timing_t *t)
{
    if (!t || t->clocks_per_cycle == 0) {
        return 0;
    }
    return (uint64_t)t->fosc_hz / (uint64_t)t->clocks_per_cycle;
}

uint64_t timing_cycles_to_ns(const timing_t *t, uint64_t cycles)
{
    uint64_t cps = timing_cycles_per_second(t);
    if (cps == 0) {
        return 0;
    }
    return mul_div_u64(cycles, 1000000000ull, cps);
}

uint64_t timing_cycles_to_us(const timing_t *t, uint64_t cycles)
{
    uint64_t cps = timing_cycles_per_second(t);
    if (cps == 0) {
        return 0;
    }
    return mul_div_u64(cycles, 1000000ull, cps);
}

uint64_t timing_ns_to_cycles(const timing_t *t, uint64_t ns)
{
    uint64_t cps = timing_cycles_per_second(t);
    if (cps == 0) {
        return 0;
    }
    return mul_div_u64(ns, cps, 1000000000ull);
}

uint64_t timing_us_to_cycles(const timing_t *t, uint64_t us)
{
    uint64_t cps = timing_cycles_per_second(t);
    if (cps == 0) {
        return 0;
    }
    return mul_div_u64(us, cps, 1000000ull);
}

void timing_step(timing_t *t, uint32_t cycles)
{
    if (!t) {
        return;
    }
    t->cycles_total += cycles;
}
