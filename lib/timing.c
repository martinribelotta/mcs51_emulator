#include "timing.h"

static uint64_t mul_div_u64(uint64_t a, uint64_t b, uint64_t c)
{
#if defined(__SIZEOF_INT128__)
    __uint128_t prod = (__uint128_t)a * (__uint128_t)b;
    return (uint64_t)(prod / c);
#else
    if (c == 0) {
        return 0;
    }

    uint64_t a_hi = a >> 32;
    uint64_t a_lo = (uint32_t)a;
    uint64_t b_hi = b >> 32;
    uint64_t b_lo = (uint32_t)b;

    uint64_t p0 = a_lo * b_lo;
    uint64_t p1 = a_lo * b_hi;
    uint64_t p2 = a_hi * b_lo;
    uint64_t p3 = a_hi * b_hi;

    uint64_t mid = (p0 >> 32) + (uint32_t)p1 + (uint32_t)p2;
    uint64_t hi = p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32);
    uint64_t lo = (mid << 32) | (uint32_t)p0;

    uint64_t q_hi = 0;
    uint64_t q_lo = 0;
    uint64_t rem = 0;

    for (int i = 127; i >= 0; --i) {
        uint64_t bit = 0;
        if (i >= 64) {
            bit = (hi >> (i - 64)) & 1u;
        } else {
            bit = (lo >> i) & 1u;
        }
        rem = (rem << 1) | bit;
        if (rem >= c) {
            rem -= c;
            if (i >= 64) {
                q_hi |= (1ull << (i - 64));
            } else {
                q_lo |= (1ull << i);
            }
        }
    }

    return q_lo;
#endif
}

void timing_init(timing_config_t *cfg, timing_state_t *state, uint32_t fosc_hz, uint8_t clocks_per_cycle)
{
    if (!cfg || !state) {
        return;
    }
    cfg->fosc_hz = fosc_hz;
    cfg->clocks_per_cycle = clocks_per_cycle ? clocks_per_cycle : 12;
    state->cycles_total = 0;
}

void timing_reset(timing_state_t *state)
{
    if (!state) {
        return;
    }
    state->cycles_total = 0;
}

void timing_set_clock(timing_config_t *cfg, uint32_t fosc_hz, uint8_t clocks_per_cycle)
{
    if (!cfg) {
        return;
    }
    cfg->fosc_hz = fosc_hz;
    cfg->clocks_per_cycle = clocks_per_cycle ? clocks_per_cycle : cfg->clocks_per_cycle;
}

uint64_t timing_cycles_per_second(const timing_config_t *cfg)
{
    if (!cfg || cfg->clocks_per_cycle == 0) {
        return 0;
    }
    return (uint64_t)cfg->fosc_hz / (uint64_t)cfg->clocks_per_cycle;
}

uint64_t timing_cycles_to_ns(const timing_config_t *cfg, uint64_t cycles)
{
    uint64_t cps = timing_cycles_per_second(cfg);
    if (cps == 0) {
        return 0;
    }
    return mul_div_u64(cycles, 1000000000ull, cps);
}

uint64_t timing_cycles_to_us(const timing_config_t *cfg, uint64_t cycles)
{
    uint64_t cps = timing_cycles_per_second(cfg);
    if (cps == 0) {
        return 0;
    }
    return mul_div_u64(cycles, 1000000ull, cps);
}

uint64_t timing_ns_to_cycles(const timing_config_t *cfg, uint64_t ns)
{
    uint64_t cps = timing_cycles_per_second(cfg);
    if (cps == 0) {
        return 0;
    }
    return mul_div_u64(ns, cps, 1000000000ull);
}

uint64_t timing_us_to_cycles(const timing_config_t *cfg, uint64_t us)
{
    uint64_t cps = timing_cycles_per_second(cfg);
    if (cps == 0) {
        return 0;
    }
    return mul_div_u64(us, cps, 1000000ull);
}

void timing_step(timing_state_t *state, uint32_t cycles)
{
    if (!state) {
        return;
    }
    state->cycles_total += cycles;
}
