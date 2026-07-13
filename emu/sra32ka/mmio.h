#ifndef MMIO_H
#define MMIO_H

#include <stdint.h>
#include <stddef.h>

typedef struct mmio_device mmio_device_t;

typedef uint32_t (*mmio_read_cb)(mmio_device_t *dev, uint32_t offset, uint32_t size);
typedef void (*mmio_write_cb)(mmio_device_t *dev, uint32_t offset, uint32_t value, uint32_t size);

struct mmio_device
{
    const char *name;
    uint32_t base;
    uint32_t size;
    void *priv;
    mmio_read_cb read;
    mmio_write_cb write;
};

int mmio_register(mmio_device_t *dev);
int mmio_read(uint32_t addr, uint32_t size, uint32_t *value);
int mmio_write(uint32_t addr, uint32_t size, uint32_t value);

#endif // MMIO_H