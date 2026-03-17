#include "timers.h"

#define SFR_TCON 0x88
#define SFR_TMOD 0x89
#define SFR_TL0  0x8A
#define SFR_TL1  0x8B
#define SFR_TH0  0x8C
#define SFR_TH1  0x8D

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
    uint32_t total = (uint32_t)tl + ticks;
    if (total > 0xFFu) {
        set_tcon_flag(cpu, tf_mask);
        uint32_t rem = total - 0x100u;
        tl = (uint8_t)(th + rem);
    } else {
        tl = (uint8_t)total;
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

void timers_init(timers_t *timers, cpu_t *cpu)
{
    if (!timers) {
        return;
    }
    timers->cpu = cpu;
    timers->acc0 = 0;
    timers->acc1 = 0;
    timers->acc0h = 0;
}

static void timers_tick_timer0(timers_t *timers, uint32_t cycles)
{
    cpu_t *cpu = timers->cpu;
    uint8_t tmod = sfr_get(cpu, SFR_TMOD);
    uint8_t tcon = sfr_get(cpu, SFR_TCON);
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

static void timers_tick_timer1(timers_t *timers, uint32_t cycles)
{
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

void timers_tick(timers_t *timers, uint32_t cycles)
{
    if (!timers || !timers->cpu) {
        return;
    }
    timers_tick_timer0(timers, cycles);
    timers_tick_timer1(timers, cycles);
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
