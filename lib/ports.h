#ifndef MCS51_PORTS_H
#define MCS51_PORTS_H

#include <stdint.h>

#include "cpu.h"

typedef uint8_t (*ports_read_cb)(uint8_t port, void *user);
typedef void (*ports_write_cb)(uint8_t port, uint8_t level, uint8_t mask, void *user);

typedef struct {
    cpu_t *cpu;
    uint8_t latch[4];
    ports_read_cb read_cb;
    ports_write_cb write_cb;
    void *user;
} ports_t;

void ports_init(ports_t *ports, cpu_t *cpu, ports_read_cb read_cb, ports_write_cb write_cb, void *user);
void ports_set_callbacks(ports_t *ports, ports_read_cb read_cb, ports_write_cb write_cb, void *user);

#endif
