#ifndef MCS51_TIMING_H
#define MCS51_TIMING_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t fosc_hz;
    uint8_t clocks_per_cycle;
    uint64_t cycles_total;
} timing_t;

void timing_init(timing_t *t, uint32_t fosc_hz, uint8_t clocks_per_cycle);
void timing_reset(timing_t *t);
void timing_set_clock(timing_t *t, uint32_t fosc_hz, uint8_t clocks_per_cycle);

uint64_t timing_cycles_per_second(const timing_t *t);
uint64_t timing_cycles_to_ns(const timing_t *t, uint64_t cycles);
uint64_t timing_cycles_to_us(const timing_t *t, uint64_t cycles);
uint64_t timing_ns_to_cycles(const timing_t *t, uint64_t ns);
uint64_t timing_us_to_cycles(const timing_t *t, uint64_t us);

void timing_step(timing_t *t, uint32_t cycles);

#endif
