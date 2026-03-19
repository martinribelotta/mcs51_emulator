#define _POSIX_C_SOURCE 200809L

#include "mcs51_emulator.h"
#include "hex_loader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

bool hex_load_file(cpu_t *cpu, const char *path);

enum { MEM_64K_SIZE = 64 * 1024 };

static uint8_t ram_read(const cpu_t *cpu, uint16_t addr, void *user)
{
    (void)cpu;
    uint8_t *ram = (uint8_t *)user;
    return ram[addr];
}

static void ram_write(cpu_t *cpu, uint16_t addr, uint8_t value, void *user)
{
    (void)cpu;
    uint8_t *ram = (uint8_t *)user;
    ram[addr] = value;
}

static void trace_stdout(cpu_t *cpu, uint16_t pc, uint8_t opcode, const char *name, void *user)
{
    FILE *out = user ? (FILE *)user : stdout;
    fprintf(out,
            "%04X  %02X  %-16s  A=%02X PSW=%02X SP=%02X DPTR=%04X\n",
            pc,
            opcode,
            name,
            cpu->acc,
            cpu->psw,
            cpu->sp,
            cpu->dptr);
}

static void print_usage(const char *prog)
{
    printf("Usage: %s <firmware.hex> [max_steps] [--trace]\n", prog);
}

static uint64_t posix_now_ns(void *user)
{
    (void)user;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static void posix_sleep_ns(uint64_t ns, void *user)
{
    (void)user;
    if (ns == 0) {
        return;
    }
    struct timespec req;
    req.tv_sec = (time_t)(ns / 1000000000ull);
    req.tv_nsec = (long)(ns % 1000000000ull);
    while (nanosleep(&req, &req) == -1 && errno == EINTR) {
        continue;
    }
}

static const cpu_t cpu_template = CPU_INIT_TEMPLATE_INIT;
static cpu_t cpu;
static uint8_t code_mem[MEM_64K_SIZE];
static uint8_t xdata_mem[MEM_64K_SIZE];
static const mem_map_region_t code_regions[] = {
    { .base = 0x0000, .size = MEM_64K_SIZE, .read = ram_read, .write = ram_write, .user = code_mem },
};
static const mem_map_region_t xdata_regions[] = {
    { .base = 0x0000, .size = MEM_64K_SIZE, .read = ram_read, .write = ram_write, .user = xdata_mem },
};
static const mem_map_t mem = {
    .code_regions = code_regions,
    .code_region_count = 1,
    .xdata_regions = xdata_regions,
    .xdata_region_count = 1,
};
static const timing_config_t timing_cfg = { .fosc_hz = 11059200u, .clocks_per_cycle = 12 };
static timing_state_t timing_state = { .cycles_total = 0 };
static const cpu_time_iface_t time_iface = {
    .now_ns = posix_now_ns,
    .sleep_ns = posix_sleep_ns,
    .user = NULL,
};
static uart_t uart;
static timers_t timers;
static ports_t ports;

typedef struct {
    bool active;
    uint8_t byte;
    uint8_t bit_index;
    uint8_t total_bits;
    uint32_t bit_cycles;
    uint32_t countdown;
    uint8_t level;
} rx_wave_t;

static rx_wave_t rx_wave;

static void timers_tick_hook(cpu_t *cpu, uint32_t cycles, void *user);
static void uart_tick_hook(cpu_t *cpu, uint32_t cycles, void *user);
static void rx_tick_hook(cpu_t *cpu, uint32_t cycles, void *user);

static const cpu_tick_entry_t tick_hooks[] = {
    { timers_tick_hook, &timers },
    { uart_tick_hook, &uart },
    { rx_tick_hook, NULL },
};

static void uart_tx_stdout(uint8_t byte, void *user)
{
    FILE *out = user ? (FILE *)user : stdout;
    fputc((int)byte, out);
    fflush(out);
}

static void uart_baud_stdout(uint32_t baud, void *user)
{
    FILE *out = user ? (FILE *)user : stdout;
    fprintf(out, "UART baud: %u\n", baud);
}

static void timers_tick_hook(cpu_t *cpu, uint32_t cycles, void *user)
{
    (void)cpu;
    timers_t *t = (timers_t *)user;
    timers_tick(t, cycles);
}

static void uart_tick_hook(cpu_t *cpu, uint32_t cycles, void *user)
{
    (void)cpu;
    uart_t *u = (uart_t *)user;
    uart_tick(u, cycles);
}

static void rx_wave_start(uint8_t byte, uint32_t bit_cycles)
{
    rx_wave.active = true;
    rx_wave.byte = byte;
    rx_wave.bit_index = 0;
    rx_wave.total_bits = 10;
    rx_wave.bit_cycles = bit_cycles;
    rx_wave.countdown = bit_cycles;
    rx_wave.level = 0;
}

static void rx_wave_tick(uint32_t cycles)
{
    if (!rx_wave.active) {
        return;
    }
    uint32_t remaining = cycles;
    while (rx_wave.active && remaining >= rx_wave.countdown) {
        remaining -= rx_wave.countdown;
        rx_wave.bit_index++;
        if (rx_wave.bit_index >= rx_wave.total_bits) {
            rx_wave.active = false;
            rx_wave.level = 1;
            break;
        }
        if (rx_wave.bit_index == 9) {
            rx_wave.level = 1;
        } else {
            uint8_t bit = (uint8_t)((rx_wave.byte >> (rx_wave.bit_index - 1)) & 0x01);
            rx_wave.level = bit;
        }
        rx_wave.countdown = rx_wave.bit_cycles;
    }
    if (rx_wave.active) {
        rx_wave.countdown -= remaining;
    }
}

static void rx_tick_hook(cpu_t *cpu, uint32_t cycles, void *user)
{
    (void)cpu;
    (void)user;
    rx_wave_tick(cycles);
}

static uint8_t ports_read_stub(uint8_t port, void *user)
{
    (void)port;
    (void)user;
    if (port == 3) {
        uint8_t level = rx_wave.active ? rx_wave.level : 1u;
        return (uint8_t)((0xFFu & (uint8_t)~0x01u) | level);
    }
    return 0xFF;
}

static void ports_write_stdout(uint8_t port, uint8_t level, uint8_t mask, void *user)
{
    if (mask == 0) {
        return;
    }
    FILE *out = user ? (FILE *)user : stdout;
    fprintf(out, "P%u: level=0x%02X mask=0x%02X\n", port, level, mask);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    cpu = cpu_template;
    mem_map_attach(&cpu, &mem);
    memset(code_mem, 0xFF, sizeof(code_mem));
    memset(xdata_mem, 0x00, sizeof(xdata_mem));

    uart_init(&uart, &timing_cfg);
    uart_set_tx_callback(&uart, uart_tx_stdout, stdout);
    uart_set_baud_callback(&uart, uart_baud_stdout, stdout);
    uart_attach(&cpu, &uart);
    timers_init(&timers, &cpu);
    ports_init(&ports, &cpu, ports_read_stub, ports_write_stdout, stdout);
    cpu_set_tick_hooks(&cpu, tick_hooks, (uint8_t)MCS51_ARRAY_LEN(tick_hooks));

    uint32_t baud = 9600u;
    uint32_t bit_cycles = (uint32_t)((timing_cycles_per_second(&timing_cfg) + baud - 1u) / baud);
    rx_wave_start(0x55, bit_cycles);

    if (!hex_load_file(&cpu, argv[1])) {
        fprintf(stderr, "Failed to load HEX file: %s\n", argv[1]);
        return 1;
    }

    uint64_t max_steps = 0;
    if (argc >= 3 && argv[2][0] != '-') {
        max_steps = (uint64_t)strtoull(argv[2], NULL, 10);
    }

    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "--trace") == 0) {
            cpu_set_trace(&cpu, true, trace_stdout, stdout);
        }
    }

    timing_reset(&timing_state);
    uint64_t wall_start_ns = posix_now_ns(NULL);
    uint64_t steps = cpu_run_timed(&cpu, max_steps, &timing_cfg, &timing_state, &time_iface);
    uint64_t wall_end_ns = posix_now_ns(NULL);
    uint64_t wall_elapsed_ns = wall_end_ns - wall_start_ns;
    uint64_t emu_elapsed_ns = timing_cycles_to_ns(&timing_cfg, timing_state.cycles_total);

    if (wall_elapsed_ns > 0) {
        double wall_ms = (double)wall_elapsed_ns / 1000000.0;
        double steps_per_sec = (double)steps * 1000000000.0 / (double)wall_elapsed_ns;
        double ns_per_step = (double)wall_elapsed_ns / (double)(steps ? steps : 1);
        double emu_ms = (double)emu_elapsed_ns / 1000000.0;
        printf("Metrics: steps=%llu wall=%.3f ms emu=%.3f ms speed=%.2f steps/s (%.1f ns/step)\n",
               (unsigned long long)steps,
               wall_ms,
               emu_ms,
               steps_per_sec,
               ns_per_step);
    } else {
        printf("Metrics: steps=%llu wall<1us emu=%llu ns\n",
               (unsigned long long)steps,
               (unsigned long long)emu_elapsed_ns);
    }

    if (cpu.halted) {
        if (cpu.halt_reason) {
            printf("CPU halted at PC=0x%04X (opcode 0x%02X): %s\n",
                   cpu.pc, cpu.last_opcode, cpu.halt_reason);
        } else {
            printf("CPU halted at PC=0x%04X (opcode 0x%02X)\n",
                   cpu.pc, cpu.last_opcode);
        }
    } else {
        printf("CPU stopped after %llu steps at PC=0x%04X\n",
               (unsigned long long)max_steps, cpu.pc);
    }

    return 0;
}
