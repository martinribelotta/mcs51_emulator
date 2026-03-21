#include "timers.h"

#include <limits.h>
#include <string.h>

#define SFR_TCON 0x88
#define SFR_TMOD 0x89
#define SFR_TL0  0x8A
#define SFR_TL1  0x8B
#define SFR_TH0  0x8C
#define SFR_TH1  0x8D
#define SFR_T2CON 0xC8
#define SFR_RCAP2L 0xCA
#define SFR_RCAP2H 0xCB
#define SFR_TL2  0xCC
#define SFR_TH2  0xCD

#define TCON_TR0 0x10
#define TCON_TF0 0x20
#define TCON_TR1 0x40
#define TCON_TF1 0x80

#define TMOD_GATE0 0x08
#define TMOD_CT0   0x04
#define TMOD_M10   0x02
#define TMOD_M00   0x01
#define TMOD_GATE1 0x80
#define TMOD_CT1   0x40
#define TMOD_M11   0x20
#define TMOD_M01   0x10

#define T2CON_TF2  0x80
#define T2CON_EXF2 0x40
#define T2CON_RCLK 0x20
#define T2CON_TCLK 0x10
#define T2CON_EXEN2 0x08
#define T2CON_TR2  0x04
#define T2CON_CT2  0x02
#define T2CON_CPRL2 0x01

static uint8_t sfr_get(const cpu_t *cpu, uint8_t addr)
{
    return cpu->sfr[(uint8_t)(addr - 0x80)];
}

static void sfr_set(cpu_t *cpu, uint8_t addr, uint8_t value)
{
    cpu->sfr[(uint8_t)(addr - 0x80)] = value;
}

static void set_tcon_flag(cpu_t *cpu, uint8_t mask)
{
    uint8_t tcon = sfr_get(cpu, SFR_TCON);
    tcon |= mask;
    sfr_set(cpu, SFR_TCON, tcon);
}

static uint8_t t0_mode(uint8_t tmod)
{
    return (uint8_t)(tmod & 0x03);
}

static uint8_t t1_mode(uint8_t tmod)
{
    return (uint8_t)((tmod >> 4) & 0x03);
}

static bool bit_array_get(const uint8_t *arr, uint8_t index)
{
    return ((arr[index >> 3] >> (index & 0x07u)) & 0x01u) != 0;
}

static void bit_array_set(uint8_t *arr, uint8_t index, bool value)
{
    uint8_t mask = (uint8_t)(1u << (index & 0x07u));
    if (value) {
        arr[index >> 3] |= mask;
    } else {
        arr[index >> 3] &= (uint8_t)~mask;
    }
}

static bool timers_event_push(timers_t *timers, timers_signal_event_t event)
{
    if (!timers || timers->event_count >= TIMERS_MAX_EVENTS) {
        return false;
    }
    timers->events[timers->event_count++] = event;
    return true;
}

static bool timers_event_pop_next_due(timers_t *timers, uint64_t target, timers_signal_event_t *out_event)
{
    if (!timers || !out_event || timers->event_count == 0) {
        return false;
    }

    int best = -1;
    uint64_t best_ts = 0;

    for (uint8_t i = 0; i < timers->event_count; ++i) {
        uint64_t ts = timers->events[i].timestamp;
        if (ts > target) {
            continue;
        }
        if (best < 0 || ts < best_ts) {
            best = (int)i;
            best_ts = ts;
        }
    }

    if (best < 0) {
        return false;
    }

    *out_event = timers->events[(uint8_t)best];
    timers->event_count--;
    timers->events[(uint8_t)best] = timers->events[timers->event_count];
    return true;
}

static void inc_mode0(cpu_t *cpu, uint8_t tl_addr, uint8_t th_addr, uint32_t ticks, uint8_t tf_mask)
{
    uint8_t tl = sfr_get(cpu, tl_addr);
    uint8_t th = sfr_get(cpu, th_addr);
    uint32_t value = ((uint32_t)th << 5) | (uint32_t)(tl & 0x1F);
    uint32_t total = value + ticks;
    if (total > 0x1FFFu) {
        set_tcon_flag(cpu, tf_mask);
    }
    uint32_t new_value = total & 0x1FFFu;
    th = (uint8_t)(new_value >> 5);
    tl = (uint8_t)((tl & 0xE0) | (new_value & 0x1Fu));
    sfr_set(cpu, th_addr, th);
    sfr_set(cpu, tl_addr, tl);
}

static void inc_mode1(cpu_t *cpu, uint8_t tl_addr, uint8_t th_addr, uint32_t ticks, uint8_t tf_mask)
{
    uint16_t value = (uint16_t)(((uint16_t)sfr_get(cpu, th_addr) << 8) | sfr_get(cpu, tl_addr));
    uint32_t total = (uint32_t)value + ticks;
    if (total > 0xFFFFu) {
        set_tcon_flag(cpu, tf_mask);
    }
    uint16_t new_value = (uint16_t)total;
    sfr_set(cpu, th_addr, (uint8_t)(new_value >> 8));
    sfr_set(cpu, tl_addr, (uint8_t)new_value);
}

static void inc_mode2(cpu_t *cpu, uint8_t tl_addr, uint8_t th_addr, uint32_t ticks, uint8_t tf_mask)
{
    uint8_t tl = sfr_get(cpu, tl_addr);
    uint8_t th = sfr_get(cpu, th_addr);
    if (ticks == 0) {
        return;
    }

    uint32_t to_first = 0x100u - (uint32_t)tl;
    if (ticks < to_first) {
        tl = (uint8_t)(tl + ticks);
    } else {
        uint32_t remaining = ticks - to_first;
        uint32_t period = 0x100u - (uint32_t)th;
        uint32_t rem = remaining % period;
        set_tcon_flag(cpu, tf_mask);
        tl = (uint8_t)((uint32_t)th + rem);
    }
    sfr_set(cpu, tl_addr, tl);
}

static void inc_mode3_8bit(cpu_t *cpu, uint8_t tl_addr, uint32_t ticks, uint8_t tf_mask)
{
    uint8_t tl = sfr_get(cpu, tl_addr);
    uint32_t total = (uint32_t)tl + ticks;
    if (total > 0xFFu) {
        set_tcon_flag(cpu, tf_mask);
    }
    tl = (uint8_t)total;
    sfr_set(cpu, tl_addr, tl);
}

static void timer2_count_ticks(timers_t *timers, uint32_t ticks)
{
    if (!timers || !timers->cpu || ticks == 0) {
        return;
    }

    cpu_t *cpu = timers->cpu;
    uint8_t t2con = sfr_get(cpu, SFR_T2CON);
    bool auto_reload = (t2con & T2CON_CPRL2) == 0;
    bool baud_mode = (t2con & (T2CON_RCLK | T2CON_TCLK)) != 0;
    bool set_tf2 = timers->profile == TIMERS_PROFILE_PRAGMATIC || !baud_mode;

    uint16_t value = (uint16_t)(((uint16_t)sfr_get(cpu, SFR_TH2) << 8) | sfr_get(cpu, SFR_TL2));
    uint16_t new_value = value;
    uint32_t overflows = 0;

    if (!auto_reload) {
        uint32_t total = (uint32_t)value + ticks;
        overflows = total >> 16;
        new_value = (uint16_t)total;
    } else {
        uint32_t to_first = 0x10000u - (uint32_t)value;
        if (ticks < to_first) {
            new_value = (uint16_t)(value + ticks);
        } else {
            uint16_t reload = (uint16_t)(((uint16_t)sfr_get(cpu, SFR_RCAP2H) << 8) | sfr_get(cpu, SFR_RCAP2L));
            uint32_t period = 0x10000u - (uint32_t)reload;
            uint32_t remaining = ticks - to_first;
            uint32_t extra = remaining / period;
            uint32_t rem = remaining % period;
            overflows = 1u + extra;
            new_value = (uint16_t)((uint32_t)reload + rem);
        }
    }

    if (overflows > 0 && set_tf2) {
        t2con |= T2CON_TF2;
    }

    sfr_set(cpu, SFR_TH2, (uint8_t)(new_value >> 8));
    sfr_set(cpu, SFR_TL2, (uint8_t)new_value);
    sfr_set(cpu, SFR_T2CON, t2con);
}

static void timer2_external_event(timers_t *timers, timers_edge_t edge)
{
    (void)edge;
    if (!timers || !timers->cpu) {
        return;
    }

    cpu_t *cpu = timers->cpu;
    uint8_t t2con = sfr_get(cpu, SFR_T2CON);
    bool tr2 = (t2con & T2CON_TR2) != 0;
    bool exen2 = (t2con & T2CON_EXEN2) != 0;
    bool capture_mode = (t2con & T2CON_CPRL2) != 0;
    bool baud_mode = (t2con & (T2CON_RCLK | T2CON_TCLK)) != 0;

    if (!tr2 || !exen2) {
        return;
    }
    if (timers->profile == TIMERS_PROFILE_STRICT_8052 && baud_mode) {
        return;
    }

    if (capture_mode) {
        uint8_t th2 = sfr_get(cpu, SFR_TH2);
        uint8_t tl2 = sfr_get(cpu, SFR_TL2);
        sfr_set(cpu, SFR_RCAP2H, th2);
        sfr_set(cpu, SFR_RCAP2L, tl2);
    } else {
        uint8_t rcap2h = sfr_get(cpu, SFR_RCAP2H);
        uint8_t rcap2l = sfr_get(cpu, SFR_RCAP2L);
        sfr_set(cpu, SFR_TH2, rcap2h);
        sfr_set(cpu, SFR_TL2, rcap2l);
    }

    t2con |= T2CON_EXF2;
    sfr_set(cpu, SFR_T2CON, t2con);
}

static void timers_dispatch_signal_event(timers_t *timers, const timers_signal_event_t *event)
{
    if (!timers || !event) {
        return;
    }

    switch (event->signal) {
    case TIMERS_SIGNAL_T0_CLK:
        timers_pulse_t0(timers, 1);
        break;
    case TIMERS_SIGNAL_T1_CLK:
        timers_pulse_t1(timers, 1);
        break;
    case TIMERS_SIGNAL_T2_CLK:
        timers_pulse_t2(timers, 1);
        break;
    case TIMERS_SIGNAL_T2_EX:
        timer2_external_event(timers, event->edge);
        break;
    default:
        break;
    }
}

void timers_init(timers_t *timers, cpu_t *cpu)
{
    if (!timers) {
        return;
    }
    memset(timers, 0, sizeof(*timers));
    timers->cpu = cpu;
    timers->profile = TIMERS_PROFILE_PRAGMATIC;
}

void timers_set_profile(timers_t *timers, timers_profile_t profile)
{
    if (!timers) {
        return;
    }
    timers->profile = profile;
}

timers_profile_t timers_get_profile(const timers_t *timers)
{
    if (!timers) {
        return TIMERS_PROFILE_PRAGMATIC;
    }
    return timers->profile;
}

void timers_route_input(timers_t *timers,
                       uint8_t input_id,
                       bool enabled,
                       timers_signal_t signal,
                       timers_edge_t edge)
{
    if (!timers || input_id >= TIMERS_MAX_INPUTS || signal >= TIMERS_SIGNAL_COUNT) {
        return;
    }
    timers->routes[input_id].enabled = enabled;
    timers->routes[input_id].signal = signal;
    timers->routes[input_id].edge = edge;
}

bool timers_emit_signal_edge(timers_t *timers, timers_signal_t signal, timers_edge_t edge, uint64_t timestamp)
{
    if (!timers || signal >= TIMERS_SIGNAL_COUNT) {
        return false;
    }

    timers_signal_event_t event = {
        .timestamp = timestamp < timers->cycles_total ? timers->cycles_total : timestamp,
        .signal = signal,
        .edge = edge,
    };
    return timers_event_push(timers, event);
}

bool timers_emit_signal_edge_now(timers_t *timers, timers_signal_t signal, timers_edge_t edge)
{
    if (!timers) {
        return false;
    }
    return timers_emit_signal_edge(timers, signal, edge, timers->cycles_total);
}

bool timers_input_sample(timers_t *timers, uint8_t input_id, bool level, uint64_t timestamp)
{
    if (!timers || input_id >= TIMERS_MAX_INPUTS) {
        return false;
    }

    bool known = bit_array_get(timers->input_known, input_id);
    bool prev = bit_array_get(timers->input_levels, input_id);
    bit_array_set(timers->input_known, input_id, true);
    bit_array_set(timers->input_levels, input_id, level);

    if (!known || prev == level) {
        return true;
    }

    timers_edge_t edge = prev ? TIMERS_EDGE_FALLING : TIMERS_EDGE_RISING;
    timers_input_route_t route = timers->routes[input_id];
    if (!route.enabled || route.edge != edge) {
        return true;
    }

    return timers_emit_signal_edge(timers, route.signal, edge, timestamp);
}

bool timers_input_sample_now(timers_t *timers, uint8_t input_id, bool level)
{
    if (!timers) {
        return false;
    }
    return timers_input_sample(timers, input_id, level, timers->cycles_total);
}

static void timers_tick_timer0_cached(timers_t *timers, uint32_t cycles, uint8_t tmod, uint8_t tcon)
{
    cpu_t *cpu = timers->cpu;
    uint8_t mode = t0_mode(tmod);
    bool gate = (tmod & TMOD_GATE0) != 0;
    bool ct = (tmod & TMOD_CT0) != 0;
    bool tr0 = (tcon & TCON_TR0) != 0;
    bool run = tr0 && (!gate || cpu->int0_level);

    if (ct || !run || cycles == 0) {
        return;
    }

    switch (mode) {
    case 0:
        inc_mode0(cpu, SFR_TL0, SFR_TH0, cycles, TCON_TF0);
        break;
    case 1:
        inc_mode1(cpu, SFR_TL0, SFR_TH0, cycles, TCON_TF0);
        break;
    case 2:
        inc_mode2(cpu, SFR_TL0, SFR_TH0, cycles, TCON_TF0);
        break;
    case 3:
        inc_mode3_8bit(cpu, SFR_TL0, cycles, TCON_TF0);
        break;
    default:
        break;
    }
}

static void timers_tick_timer1_cached(timers_t *timers, uint32_t cycles, uint8_t tmod, uint8_t tcon)
{
    cpu_t *cpu = timers->cpu;
    uint8_t t0mode = t0_mode(tmod);
    uint8_t mode = t1_mode(tmod);
    bool gate = (tmod & TMOD_GATE1) != 0;
    bool ct = (tmod & TMOD_CT1) != 0;
    bool tr1 = (tcon & TCON_TR1) != 0;
    bool run = tr1 && (!gate || cpu->int1_level);

    if (t0mode == 3) {
        if (ct || !run || cycles == 0) {
            return;
        }
        inc_mode3_8bit(cpu, SFR_TH0, cycles, TCON_TF1);
        return;
    }

    if (ct || !run || cycles == 0) {
        return;
    }

    switch (mode) {
    case 0:
        inc_mode0(cpu, SFR_TL1, SFR_TH1, cycles, TCON_TF1);
        break;
    case 1:
        inc_mode1(cpu, SFR_TL1, SFR_TH1, cycles, TCON_TF1);
        break;
    case 2:
        inc_mode2(cpu, SFR_TL1, SFR_TH1, cycles, TCON_TF1);
        break;
    default:
        break;
    }
}

static void timers_tick_timer2_cached(timers_t *timers, uint32_t cycles, uint8_t t2con)
{
    bool tr2 = (t2con & T2CON_TR2) != 0;
    bool ct2 = (t2con & T2CON_CT2) != 0;

    if (!tr2 || ct2 || cycles == 0) {
        return;
    }

    timer2_count_ticks(timers, cycles);
}

static void timers_advance_cycles(timers_t *timers, uint64_t delta)
{
    cpu_t *cpu = timers->cpu;
    while (delta > 0) {
        uint32_t chunk = delta > UINT_MAX ? UINT_MAX : (uint32_t)delta;
        uint8_t tmod = sfr_get(cpu, SFR_TMOD);
        uint8_t tcon = sfr_get(cpu, SFR_TCON);
        uint8_t t2con = sfr_get(cpu, SFR_T2CON);

        timers_tick_timer0_cached(timers, chunk, tmod, tcon);
        timers_tick_timer1_cached(timers, chunk, tmod, tcon);
        timers_tick_timer2_cached(timers, chunk, t2con);
        delta -= chunk;
    }
}

void timers_tick(timers_t *timers, uint32_t cycles)
{
    if (!timers || !timers->cpu) {
        return;
    }

    uint64_t target = timers->cycles_total + (uint64_t)cycles;
    timers_signal_event_t event;

    while (timers_event_pop_next_due(timers, target, &event)) {
        if (event.timestamp > timers->cycles_total) {
            uint64_t delta = event.timestamp - timers->cycles_total;
            timers_advance_cycles(timers, delta);
            timers->cycles_total = event.timestamp;
        }
        timers_dispatch_signal_event(timers, &event);
    }

    if (target > timers->cycles_total) {
        uint64_t delta = target - timers->cycles_total;
        timers_advance_cycles(timers, delta);
        timers->cycles_total = target;
    }
}

void timers_pulse_t0(timers_t *timers, uint32_t pulses)
{
    if (!timers || !timers->cpu || pulses == 0) {
        return;
    }
    cpu_t *cpu = timers->cpu;
    uint8_t tmod = sfr_get(cpu, SFR_TMOD);
    uint8_t tcon = sfr_get(cpu, SFR_TCON);
    uint8_t mode = t0_mode(tmod);
    bool gate = (tmod & TMOD_GATE0) != 0;
    bool ct = (tmod & TMOD_CT0) != 0;
    bool tr0 = (tcon & TCON_TR0) != 0;
    bool run = tr0 && (!gate || cpu->int0_level);

    if (!ct || !run) {
        return;
    }

    switch (mode) {
    case 0:
        inc_mode0(cpu, SFR_TL0, SFR_TH0, pulses, TCON_TF0);
        break;
    case 1:
        inc_mode1(cpu, SFR_TL0, SFR_TH0, pulses, TCON_TF0);
        break;
    case 2:
        inc_mode2(cpu, SFR_TL0, SFR_TH0, pulses, TCON_TF0);
        break;
    case 3:
        inc_mode3_8bit(cpu, SFR_TL0, pulses, TCON_TF0);
        break;
    default:
        break;
    }
}

void timers_pulse_t1(timers_t *timers, uint32_t pulses)
{
    if (!timers || !timers->cpu || pulses == 0) {
        return;
    }
    cpu_t *cpu = timers->cpu;
    uint8_t tmod = sfr_get(cpu, SFR_TMOD);
    uint8_t tcon = sfr_get(cpu, SFR_TCON);
    uint8_t t0mode = t0_mode(tmod);
    uint8_t mode = t1_mode(tmod);
    bool gate = (tmod & TMOD_GATE1) != 0;
    bool ct = (tmod & TMOD_CT1) != 0;
    bool tr1 = (tcon & TCON_TR1) != 0;
    bool run = tr1 && (!gate || cpu->int1_level);

    if (t0mode == 3) {
        if (!ct || !run) {
            return;
        }
        inc_mode3_8bit(cpu, SFR_TH0, pulses, TCON_TF1);
        return;
    }

    if (!ct || !run) {
        return;
    }

    switch (mode) {
    case 0:
        inc_mode0(cpu, SFR_TL1, SFR_TH1, pulses, TCON_TF1);
        break;
    case 1:
        inc_mode1(cpu, SFR_TL1, SFR_TH1, pulses, TCON_TF1);
        break;
    case 2:
        inc_mode2(cpu, SFR_TL1, SFR_TH1, pulses, TCON_TF1);
        break;
    default:
        break;
    }
}

void timers_pulse_t2(timers_t *timers, uint32_t pulses)
{
    if (!timers || !timers->cpu || pulses == 0) {
        return;
    }
    cpu_t *cpu = timers->cpu;
    uint8_t t2con = sfr_get(cpu, SFR_T2CON);
    bool tr2 = (t2con & T2CON_TR2) != 0;
    bool ct2 = (t2con & T2CON_CT2) != 0;

    if (!tr2 || !ct2) {
        return;
    }

    timer2_count_ticks(timers, pulses);
}
