#ifndef MCS51_TIMERS_H
#define MCS51_TIMERS_H

#include <stdint.h>

#include "cpu.h"

typedef struct {
    cpu_t *cpu;
    uint32_t acc0;
    uint32_t acc1;
    uint32_t acc0h;
} timers_t;

void timers_init(timers_t *timers, cpu_t *cpu);
void timers_tick(timers_t *timers, uint32_t cycles);
void timers_pulse_t0(timers_t *timers, uint32_t pulses);
void timers_pulse_t1(timers_t *timers, uint32_t pulses);

#endif
