#include "cpu.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

static int hex_value(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return 10 + (c - 'a');
    }
    if (c >= 'A' && c <= 'F') {
        return 10 + (c - 'A');
    }
    return -1;
}

static bool parse_hex_byte(const char *s, uint8_t *out)
{
    int hi = hex_value(s[0]);
    int lo = hex_value(s[1]);
    if (hi < 0 || lo < 0) {
        return false;
    }
    *out = (uint8_t)((hi << 4) | lo);
    return true;
}

bool hex_load_file(cpu_t *cpu, const char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp) {
        return false;
    }

    char line[1024];
    uint32_t base = 0;

    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }
        if (len == 0) {
            continue;
        }
        if (line[0] != ':') {
            fclose(fp);
            return false;
        }

        uint8_t count = 0;
        uint8_t addr_hi = 0;
        uint8_t addr_lo = 0;
        uint8_t type = 0;

        if (!parse_hex_byte(&line[1], &count) ||
            !parse_hex_byte(&line[3], &addr_hi) ||
            !parse_hex_byte(&line[5], &addr_lo) ||
            !parse_hex_byte(&line[7], &type)) {
            fclose(fp);
            return false;
        }

        uint16_t addr = (uint16_t)((addr_hi << 8) | addr_lo);

        if (type == 0x00) {
            for (uint8_t i = 0; i < count; ++i) {
                uint8_t data = 0;
                if (!parse_hex_byte(&line[9 + (i * 2)], &data)) {
                    fclose(fp);
                    return false;
                }
                uint32_t target = base + addr + i;
                if (target < sizeof(cpu->code)) {
                    cpu->code[target] = data;
                }
            }
        } else if (type == 0x01) {
            break;
        } else if (type == 0x04) {
            uint8_t hi = 0;
            uint8_t lo = 0;
            if (!parse_hex_byte(&line[9], &hi) || !parse_hex_byte(&line[11], &lo)) {
                fclose(fp);
                return false;
            }
            base = (uint32_t)((hi << 8) | lo) << 16;
        } else {
            continue;
        }
    }

    fclose(fp);
    return true;
}
