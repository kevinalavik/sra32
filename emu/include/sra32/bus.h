#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct bus bus_t;

struct bus
{
    bool (*read8)(bus_t *bus, uint32_t addr, uint8_t *out);
    bool (*write8)(bus_t *bus, uint32_t addr, uint8_t value);
    bool (*read32)(bus_t *bus, uint32_t addr, uint32_t *out);
    bool (*write32)(bus_t *bus, uint32_t addr, uint32_t value);
    uint32_t fault_addr;
};

bool bus_read8(bus_t *bus, uint32_t addr, uint8_t *out);
bool bus_write8(bus_t *bus, uint32_t addr, uint8_t value);
bool bus_read32(bus_t *bus, uint32_t addr, uint32_t *out);
bool bus_write32(bus_t *bus, uint32_t addr, uint32_t value);

typedef struct ram
{
    bus_t bus;
    uint8_t *mem;
    size_t size;
} ram_t;

bool ram_init(ram_t *ram, size_t size);
void ram_destroy(ram_t *ram);
bool ram_load(ram_t *ram, const uint8_t *data, size_t len, uint32_t base);
