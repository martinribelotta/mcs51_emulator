#ifndef MCS51_UART_H
#define MCS51_UART_H

#include <stdint.h>
#include <stdbool.h>

#include "cpu.h"
#include "timing.h"

typedef void (*uart_tx_byte_fn)(uint8_t byte, void *user);

typedef struct {
    const timing_config_t *timing_cfg;
    cpu_t *cpu;
    uint32_t bit_cycles;
    uint32_t bit_acc;
    bool tx_busy;
    uint8_t tx_byte;
    uint8_t tx_bits_remaining;
    bool rx_pending;
    uint8_t rx_byte;
    uint32_t rx_cycles_remaining;
    bool rx_overrun;
    uart_tx_byte_fn tx_cb;
    void *tx_user;
} uart_t;

void uart_init(uart_t *uart, const timing_config_t *timing_cfg);
void uart_attach(cpu_t *cpu, uart_t *uart);
void uart_tick(uart_t *uart, uint32_t cycles);
void uart_set_tx_callback(uart_t *uart, uart_tx_byte_fn fn, void *user);
bool uart_queue_rx_byte(uart_t *uart, uint8_t byte);

#endif
