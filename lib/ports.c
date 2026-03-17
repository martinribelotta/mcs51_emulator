#include "ports.h"

#define SFR_P0 0x80
#define SFR_P1 0x90
#define SFR_P2 0xA0
#define SFR_P3 0xB0

static uint8_t sfr_get(const cpu_t *cpu, uint8_t addr)
{
    return cpu->sfr[(uint8_t)(addr - 0x80)];
}

static void sfr_set(cpu_t *cpu, uint8_t addr, uint8_t value)
{
    cpu->sfr[(uint8_t)(addr - 0x80)] = value;
}

static uint8_t port_pullup_mask(uint8_t port)
{
    return port == 0 ? 0x00 : 0xFF;
}

static uint8_t port_read_effective(ports_t *ports, uint8_t port)
{
    uint8_t latch = ports->latch[port];
    uint8_t ext = ports->read_cb ? ports->read_cb(port, ports->user) : 0x00;
    uint8_t pull = port_pullup_mask(port);
    return (uint8_t)(latch & (uint8_t)(ext | pull));
}

static void port_notify_write(ports_t *ports, uint8_t port)
{
    if (!ports->write_cb) {
        return;
    }
    uint8_t latch = ports->latch[port];
    uint8_t pull = port_pullup_mask(port);
    uint8_t driven_mask = (uint8_t)((uint8_t)~latch | (latch & pull));
    uint8_t driven_level = (uint8_t)(latch & pull);
    if (driven_mask == 0) {
        return;
    }
    ports->write_cb(port, driven_level, driven_mask, ports->user);
}

static uint8_t ports_read_sfr(const cpu_t *cpu, uint8_t addr, void *user)
{
    (void)addr;
    ports_t *ports = (ports_t *)user;
    uint8_t port = 0;
    switch (addr) {
    case SFR_P0: port = 0; break;
    case SFR_P1: port = 1; break;
    case SFR_P2: port = 2; break;
    case SFR_P3: port = 3; break;
    default: return sfr_get(cpu, addr);
    }
    return port_read_effective(ports, port);
}

static void ports_write_sfr(cpu_t *cpu, uint8_t addr, uint8_t value, void *user)
{
    ports_t *ports = (ports_t *)user;
    uint8_t port = 0;
    switch (addr) {
    case SFR_P0: port = 0; break;
    case SFR_P1: port = 1; break;
    case SFR_P2: port = 2; break;
    case SFR_P3: port = 3; break;
    default:
        return;
    }
    ports->latch[port] = value;
    sfr_set(cpu, addr, value);
    port_notify_write(ports, port);
}

void ports_init(ports_t *ports, cpu_t *cpu, ports_read_cb read_cb, ports_write_cb write_cb, void *user)
{
    if (!ports || !cpu) {
        return;
    }
    ports->cpu = cpu;
    ports->read_cb = read_cb;
    ports->write_cb = write_cb;
    ports->user = user;

    ports->latch[0] = sfr_get(cpu, SFR_P0);
    ports->latch[1] = sfr_get(cpu, SFR_P1);
    ports->latch[2] = sfr_get(cpu, SFR_P2);
    ports->latch[3] = sfr_get(cpu, SFR_P3);

    cpu_set_sfr_hook(cpu, SFR_P0, ports_read_sfr, ports_write_sfr, ports);
    cpu_set_sfr_hook(cpu, SFR_P1, ports_read_sfr, ports_write_sfr, ports);
    cpu_set_sfr_hook(cpu, SFR_P2, ports_read_sfr, ports_write_sfr, ports);
    cpu_set_sfr_hook(cpu, SFR_P3, ports_read_sfr, ports_write_sfr, ports);
}

void ports_set_callbacks(ports_t *ports, ports_read_cb read_cb, ports_write_cb write_cb, void *user)
{
    if (!ports) {
        return;
    }
    ports->read_cb = read_cb;
    ports->write_cb = write_cb;
    ports->user = user;
}
