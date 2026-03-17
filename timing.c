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
