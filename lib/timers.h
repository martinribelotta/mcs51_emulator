#ifndef MCS51_TIMERS_H
#define MCS51_TIMERS_H

#include <stdbool.h>
#include <stdint.h>

#include "cpu.h"

#define TIMERS_MAX_INPUTS 16u
#define TIMERS_MAX_EVENTS 64u

typedef enum {
    TIMERS_SIGNAL_T0_CLK = 0,
    TIMERS_SIGNAL_T1_CLK = 1,
    TIMERS_SIGNAL_T2_CLK = 2,
    TIMERS_SIGNAL_T2_EX = 3,
    TIMERS_SIGNAL_COUNT
} timers_signal_t;

typedef enum {
    TIMERS_EDGE_RISING = 0,
    TIMERS_EDGE_FALLING = 1
} timers_edge_t;

typedef enum {
    TIMERS_PROFILE_STRICT_8052 = 0,
    TIMERS_PROFILE_PRAGMATIC = 1
} timers_profile_t;

typedef struct {
    bool enabled;
    timers_signal_t signal;
    timers_edge_t edge;
} timers_input_route_t;

typedef struct {
    uint64_t timestamp;
    timers_signal_t signal;
    timers_edge_t edge;
} timers_signal_event_t;

typedef struct {
    cpu_t *cpu;
    uint64_t cycles_total;
    timers_profile_t profile;
    timers_input_route_t routes[TIMERS_MAX_INPUTS];
    timers_signal_event_t events[TIMERS_MAX_EVENTS];
    uint8_t event_count;
    uint8_t input_levels[(TIMERS_MAX_INPUTS + 7u) / 8u];
    uint8_t input_known[(TIMERS_MAX_INPUTS + 7u) / 8u];
} timers_t;

void timers_init(timers_t *timers, cpu_t *cpu);
void timers_set_profile(timers_t *timers, timers_profile_t profile);
timers_profile_t timers_get_profile(const timers_t *timers);

void timers_route_input(timers_t *timers,
                       uint8_t input_id,
                       bool enabled,
                       timers_signal_t signal,
                       timers_edge_t edge);

bool timers_input_sample(timers_t *timers, uint8_t input_id, bool level, uint64_t timestamp);
bool timers_input_sample_now(timers_t *timers, uint8_t input_id, bool level);
bool timers_emit_signal_edge(timers_t *timers, timers_signal_t signal, timers_edge_t edge, uint64_t timestamp);
bool timers_emit_signal_edge_now(timers_t *timers, timers_signal_t signal, timers_edge_t edge);

void timers_tick(timers_t *timers, uint32_t cycles);
void timers_pulse_t0(timers_t *timers, uint32_t pulses);
void timers_pulse_t1(timers_t *timers, uint32_t pulses);
void timers_pulse_t2(timers_t *timers, uint32_t pulses);

#endif
