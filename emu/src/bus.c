#include <sra32/bus.h>
#include <util/bit.h>
#include <util/log.h>

#include <stdlib.h>
#include <string.h>

bool bus_read8(bus_t *bus, uint32_t addr, uint8_t *out)
{
    return bus->read8(bus, addr, out);
}

bool bus_write8(bus_t *bus, uint32_t addr, uint8_t value)
{
    return bus->write8(bus, addr, value);
}

bool bus_read32(bus_t *bus, uint32_t addr, uint32_t *out)
{
    if (bus->read32)
        return bus->read32(bus, addr, out);

    uint8_t b[4];
    for (uint32_t i = 0; i < 4; i++)
        if (!bus->read8(bus, addr + i, &b[i]))
            return false;

    *out = LE32_LOAD(b);
    return true;
}

bool bus_write32(bus_t *bus, uint32_t addr, uint32_t value)
{
    if (bus->write32)
        return bus->write32(bus, addr, value);

    for (uint32_t i = 0; i < 4; i++)
        if (!bus->write8(bus, addr + i, BYTE(value, i)))
            return false;
    return true;
}

/* ram device */
static bool ram_check(ram_t *ram, uint32_t addr, size_t len)
{
    if ((uint64_t)addr + len > ram->size)
    {
        ram->bus.fault_addr = addr;
        return false;
    }
    return true;
}

static bool ram_read8(bus_t *bus, uint32_t addr, uint8_t *out)
{
    ram_t *ram = (ram_t *)bus;
    if (!ram_check(ram, addr, 1))
        return false;
    *out = ram->mem[addr];
    return true;
}

static bool ram_write8(bus_t *bus, uint32_t addr, uint8_t value)
{
    ram_t *ram = (ram_t *)bus;
    if (!ram_check(ram, addr, 1))
        return false;
    ram->mem[addr] = value;
    return true;
}

static bool ram_read32(bus_t *bus, uint32_t addr, uint32_t *out)
{
    ram_t *ram = (ram_t *)bus;
    if (!ram_check(ram, addr, 4))
        return false;

    *out = LE32_LOAD(&ram->mem[addr]);
    return true;
}

static bool ram_write32(bus_t *bus, uint32_t addr, uint32_t value)
{
    ram_t *ram = (ram_t *)bus;
    if (!ram_check(ram, addr, 4))
        return false;

    LE32_STORE(&ram->mem[addr], value);
    return true;
}

bool ram_init(ram_t *ram, size_t size)
{
    ram->mem = calloc(size, 1);
    if (!ram->mem)
    {
        log_error("failed to allocate %zu byte(s) of ram", size);
        return false;
    }

    ram->size = size;
    ram->bus.read8 = ram_read8;
    ram->bus.write8 = ram_write8;
    ram->bus.read32 = ram_read32;
    ram->bus.write32 = ram_write32;
    ram->bus.fault_addr = 0;
    return true;
}

void ram_destroy(ram_t *ram)
{
    free(ram->mem);
    ram->mem = NULL;
    ram->size = 0;
}

bool ram_load(ram_t *ram, const uint8_t *data, size_t len, uint32_t base)
{
    if (!ram_check(ram, base, len))
    {
        log_error("load of %zu byte(s) outside memory: 0x%08x", len, base);
        return false;
    }
    memcpy(ram->mem + base, data, len);
    return true;
}
