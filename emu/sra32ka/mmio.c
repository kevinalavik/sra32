#include "mmio.h"

#define MAX_MMIO_DEVICES 32

static mmio_device_t *devices[MAX_MMIO_DEVICES];
static size_t device_count;

int mmio_register(mmio_device_t *dev)
{
    if (device_count >= MAX_MMIO_DEVICES)
        return -1;

    devices[device_count++] = dev;
    return 0;
}

static mmio_device_t *find_device(uint32_t addr)
{
    for (size_t i = 0; i < device_count; i++)
    {
        mmio_device_t *dev = devices[i];
        if (addr >= dev->base &&
            addr < dev->base + dev->size)
        {
            return dev;
        }
    }

    return NULL;
}

int mmio_read(uint32_t addr, uint32_t size, uint32_t *value)
{
    mmio_device_t *dev = find_device(addr);
    if (!dev || !dev->read)
        return -1;

    *value = dev->read(dev, addr - dev->base, size);
    return 0;
}

int mmio_write(uint32_t addr, uint32_t size, uint32_t value)
{
    mmio_device_t *dev = find_device(addr);

    if (!dev || !dev->write)
        return -1;

    dev->write(dev, addr - dev->base, value, size);
    return 0;
}