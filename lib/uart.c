#include "uart.h"

#include <string.h>

#define SFR_PCON 0x87
#define SFR_TCON 0x88
#define SFR_TMOD 0x89
#define SFR_TL1  0x8B
#define SFR_TH1  0x8D
#define SFR_SCON 0x98
#define SFR_SBUF 0x99

#define PCON_SMOD 0x80
#define TCON_TR1  0x40

#define SCON_SM0  0x80
#define SCON_SM1  0x40
#define SCON_SM2  0x20
#define SCON_REN  0x10
#define SCON_TB8  0x08
#define SCON_RB8  0x04
#define SCON_TI   0x02
#define SCON_RI   0x01

static uint8_t sfr_get(const cpu_t *cpu, uint8_t addr)
{
    return cpu->sfr[(uint8_t)(addr - 0x80)];
}

static void sfr_set(cpu_t *cpu, uint8_t addr, uint8_t value)
{
    cpu->sfr[(uint8_t)(addr - 0x80)] = value;
}

static void uart_update_baud(uart_t *uart)
{
    if (!uart || !uart->cpu) {
        return;
    }
    uint8_t scon = sfr_get(uart->cpu, SFR_SCON);
    uint8_t pcon = sfr_get(uart->cpu, SFR_PCON);
    uint8_t tcon = sfr_get(uart->cpu, SFR_TCON);
    uint8_t th1 = sfr_get(uart->cpu, SFR_TH1);

    uint8_t sm0 = (scon & SCON_SM0) != 0;
    uint8_t sm1 = (scon & SCON_SM1) != 0;
    bool tr1 = (tcon & TCON_TR1) != 0;

    uint32_t prev_baud = uart->baud;
    uart->baud = 0;
    if (sm0 && !sm1) {
        uint32_t baud = uart->timing_cfg ? uart->timing_cfg->fosc_hz / 64u : 0;
        if (pcon & PCON_SMOD) {
            baud *= 2u;
        }
        if (baud == 0) {
            uart->bit_cycles = 0;
            return;
        }
        uart->baud = baud;
        uint64_t cps = timing_cycles_per_second(uart->timing_cfg);
        uart->bit_cycles = (uint32_t)((cps + baud - 1) / baud);
        if (uart->bit_cycles == 0) {
            uart->bit_cycles = 1;
        }
        if (uart->baud != prev_baud && uart->baud_cb) {
            uart->baud_cb(uart->baud, uart->tx_user);
        }
        return;
    }

    if (!sm0 && sm1) {
        if (!tr1) {
            uart->bit_cycles = 0;
            return;
        }
        uint32_t reload = (uint32_t)(256u - (uint32_t)th1);
        uint32_t cycles = 32u * reload;
        if (pcon & PCON_SMOD) {
            cycles >>= 1;
        }
        if (cycles == 0) {
            cycles = 1;
        }
        uart->bit_cycles = cycles;
        uint64_t cps = timing_cycles_per_second(uart->timing_cfg);
        uart->baud = cps / (uint64_t)uart->bit_cycles;
        if (uart->baud != prev_baud && uart->baud_cb) {
            uart->baud_cb(uart->baud, uart->tx_user);
        }
        return;
    }

    if (sm0 && sm1) {
        if (!tr1) {
            uart->bit_cycles = 0;
            return;
        }
        uint32_t reload = (uint32_t)(256u - (uint32_t)th1);
        uint32_t cycles = 32u * reload;
        if (pcon & PCON_SMOD) {
            cycles >>= 1;
        }
        if (cycles == 0) {
            cycles = 1;
        }
        uart->bit_cycles = cycles;
        uint64_t cps = timing_cycles_per_second(uart->timing_cfg);
        uart->baud = cps / (uint64_t)uart->bit_cycles;
        if (uart->baud != prev_baud && uart->baud_cb) {
            uart->baud_cb(uart->baud, uart->tx_user);
        }
        return;
    }

    uart->bit_cycles = 0;
    if (uart->baud != prev_baud && uart->baud_cb) {
        uart->baud_cb(uart->baud, uart->tx_user);
    }
}

static void uart_set_ti(cpu_t *cpu, bool value)
{
    uint8_t scon = sfr_get(cpu, SFR_SCON);
    if (value) {
        scon |= SCON_TI;
    } else {
        scon &= (uint8_t)~SCON_TI;
    }
    sfr_set(cpu, SFR_SCON, scon);
}

static void uart_set_ri(cpu_t *cpu, bool value)
{
    uint8_t scon = sfr_get(cpu, SFR_SCON);
    if (value) {
        scon |= SCON_RI;
    } else {
        scon &= (uint8_t)~SCON_RI;
    }
    sfr_set(cpu, SFR_SCON, scon);
}

static uint8_t uart_read_sfr(const cpu_t *cpu, uint8_t addr, void *user)
{
    (void)user;
    return sfr_get(cpu, addr);
}

static void uart_write_sfr(cpu_t *cpu, uint8_t addr, uint8_t value, void *user)
{
    uart_t *uart = (uart_t *)user;
    sfr_set(cpu, addr, value);
    switch (addr) {
    case SFR_SCON:
    case SFR_PCON:
    case SFR_TCON:
    case SFR_TMOD:
    case SFR_TL1:
    case SFR_TH1:
        uart_update_baud(uart);
        break;
    default:
        break;
    }
}

static uint8_t uart_read_sbuf(const cpu_t *cpu, uint8_t addr, void *user)
{
    (void)addr;
    (void)user;
    return sfr_get(cpu, SFR_SBUF);
}

static void uart_write_sbuf(cpu_t *cpu, uint8_t addr, uint8_t value, void *user)
{
    (void)addr;
    uart_t *uart = (uart_t *)user;
    sfr_set(cpu, SFR_SBUF, value);
    uart->tx_byte = value;
    uart->tx_bit9 = (sfr_get(cpu, SFR_SCON) & SCON_TB8) ? 1u : 0u;
    uart_set_ti(cpu, false);
    uart_update_baud(uart);
    if (uart->bit_cycles == 0) {
        return;
    }
    uart->tx_busy = true;
    uart->tx_bits_remaining = (uint8_t)((sfr_get(cpu, SFR_SCON) & SCON_SM0) ? 11 : 10);
    uart->bit_acc = 0;
}

void uart_init(uart_t *uart, const timing_config_t *timing_cfg)
{
    if (!uart) {
        return;
    }
    memset(uart, 0, sizeof(*uart));
    uart->timing_cfg = timing_cfg;
}

void uart_attach(cpu_t *cpu, uart_t *uart)
{
    if (!cpu || !uart) {
        return;
    }
    uart->cpu = cpu;
    cpu_set_sfr_hook(cpu, SFR_SCON, uart_read_sfr, uart_write_sfr, uart);
    cpu_set_sfr_hook(cpu, SFR_SBUF, uart_read_sbuf, uart_write_sbuf, uart);
    cpu_set_sfr_hook(cpu, SFR_PCON, uart_read_sfr, uart_write_sfr, uart);
    cpu_set_sfr_hook(cpu, SFR_TCON, uart_read_sfr, uart_write_sfr, uart);
    cpu_set_sfr_hook(cpu, SFR_TMOD, uart_read_sfr, uart_write_sfr, uart);
    cpu_set_sfr_hook(cpu, SFR_TL1, uart_read_sfr, uart_write_sfr, uart);
    cpu_set_sfr_hook(cpu, SFR_TH1, uart_read_sfr, uart_write_sfr, uart);
    uart_update_baud(uart);
}

void uart_tick(uart_t *uart, uint32_t cycles)
{
    if (!uart || !uart->cpu || uart->bit_cycles == 0) {
        return;
    }

    uint8_t scon = sfr_get(uart->cpu, SFR_SCON);
    if ((scon & SCON_REN) == 0) {
        uart->rx_pending = false;
    }

    if (uart->tx_busy) {
        uart->bit_acc += cycles;
        while (uart->bit_acc >= uart->bit_cycles && uart->tx_bits_remaining > 0) {
            uart->bit_acc -= uart->bit_cycles;
            uart->tx_bits_remaining--;
            if (uart->tx_bits_remaining == 0) {
                uart->tx_busy = false;
                uart_set_ti(uart->cpu, true);
                if (uart->tx_cb9) {
                    uart->tx_cb9(uart->tx_byte, uart->tx_bit9, uart->baud, uart->tx_user);
                } else if (uart->tx_cb) {
                    uart->tx_cb(uart->tx_byte, uart->tx_user);
                }
            }
        }
    }

    if (uart->rx_pending) {
        if (uart->rx_cycles_remaining > cycles) {
            uart->rx_cycles_remaining -= cycles;
        } else {
            uart->rx_cycles_remaining = 0;
        }
        if (uart->rx_cycles_remaining == 0) {
            uint8_t scon = sfr_get(uart->cpu, SFR_SCON);
            if (scon & SCON_SM2) {
                if (uart->rx_bit9 == 0) {
                    uart->rx_pending = false;
                    return;
                }
            }
            if (scon & SCON_RI) {
                uart->rx_overrun = true;
            } else {
                if (uart->rx_bit9) {
                    scon |= SCON_RB8;
                } else {
                    scon &= (uint8_t)~SCON_RB8;
                }
                sfr_set(uart->cpu, SFR_SCON, scon);
                sfr_set(uart->cpu, SFR_SBUF, uart->rx_byte);
                uart_set_ri(uart->cpu, true);
            }
            uart->rx_pending = false;
        }
    }
}

void uart_set_tx_callback(uart_t *uart, uart_tx_byte_fn fn, void *user)
{
    if (!uart) {
        return;
    }
    uart->tx_cb = fn;
    uart->tx_user = user;
}

void uart_set_tx_callback9(uart_t *uart, uart_tx_frame_fn fn, void *user)
{
    if (!uart) {
        return;
    }
    uart->tx_cb9 = fn;
    uart->tx_user = user;
}

void uart_set_baud_callback(uart_t *uart, uart_baud_change_fn fn, void *user)
{
    if (!uart) {
        return;
    }
    uart->baud_cb = fn;
    uart->tx_user = user;
}

bool uart_queue_rx_byte(uart_t *uart, uint8_t byte)
{
    if (!uart || uart->bit_cycles == 0) {
        return false;
    }
    if (uart->rx_pending) {
        uart->rx_overrun = true;
        return false;
    }
    uart->rx_pending = true;
    uart->rx_byte = byte;
    uart->rx_bit9 = 0;
    uart->rx_cycles_remaining = uart->bit_cycles * (uint32_t)((sfr_get(uart->cpu, SFR_SCON) & SCON_SM0) ? 11 : 10);
    return true;
}

bool uart_queue_rx_frame(uart_t *uart, uint8_t byte, uint8_t bit9)
{
    if (!uart || uart->bit_cycles == 0) {
        return false;
    }
    if (uart->rx_pending) {
        uart->rx_overrun = true;
        return false;
    }
    uart->rx_pending = true;
    uart->rx_byte = byte;
    uart->rx_bit9 = (uint8_t)(bit9 ? 1u : 0u);
    uart->rx_cycles_remaining = uart->bit_cycles * (uint32_t)((sfr_get(uart->cpu, SFR_SCON) & SCON_SM0) ? 11 : 10);
    return true;
}

uint32_t uart_get_baud(const uart_t *uart)
{
    if (!uart) {
        return 0;
    }
    return uart->baud;
}
