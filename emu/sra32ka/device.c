#include "device.h"
#include "mmio.h"
#include "../log.h"

#include <string.h>

/*
 * read only from the guest:
 *
 *       +0x00          device_count (u32)
 *       +0x20 + i*0x20 entry i:
 *       +0x00          name[16]    nul-padded ascii
 *       +0x10          parent      index of parent entry, or DEVICE_NONE
 *       +0x14          base        mmio base of this device's own registers, or 0
 *       +0x18          (reserved)
 *       +0x1C          (reserved)
 */
#define DEVICE_ENTRY_SIZE 32
#define DEVICE_ENTRIES_OFFSET 32

typedef struct
{
    char name[DEVICE_NAME_MAX];
    uint32_t parent;
    uint32_t base;
    uint32_t reserved[2];
} device_entry_t;

static device_entry_t entries[DEVICE_MAX];
static uint32_t entry_count;

static uint8_t device_tree_byte(uint32_t off)
{
    if (off < 4)
        return (uint8_t)(entry_count >> (8 * off));

    if (off >= DEVICE_ENTRIES_OFFSET)
    {
        uint32_t rel = off - DEVICE_ENTRIES_OFFSET;
        uint32_t idx = rel / DEVICE_ENTRY_SIZE;
        uint32_t field = rel % DEVICE_ENTRY_SIZE;

        if (idx < entry_count)
            return ((const uint8_t *)&entries[idx])[field];
    }

    return 0;
}

static uint32_t device_tree_read(mmio_device_t *dev, uint32_t off, uint32_t size)
{
    (void)dev;
    uint32_t value = 0, i;
    for (i = 0; i < size; i++)
        value |= (uint32_t)device_tree_byte(off + i) << (8 * i);
    return value;
}

static void device_tree_write(mmio_device_t *dev, uint32_t off, uint32_t value, uint32_t size)
{
    (void)dev;
    (void)off;
    (void)value;
    (void)size;
}

void device_tree_init(void)
{
    static mmio_device_t dev;
    entry_count = 0;
    dev.name = "devtree";
    dev.base = DEVICE_TREE_BASE;
    dev.size = DEVICE_ENTRIES_OFFSET + DEVICE_MAX * DEVICE_ENTRY_SIZE;
    dev.priv = NULL;
    dev.read = device_tree_read;
    dev.write = device_tree_write;
    if (mmio_register(&dev) != 0)
        log_error("devtree", "failed to register device tree");
}

uint32_t device_tree_add(const char *name, uint32_t parent, uint32_t base)
{
    device_entry_t *e;
    if (entry_count >= DEVICE_MAX)
    {
        log_error("devtree", "device table full, dropping '%s'", name);
        return DEVICE_NONE;
    }

    e = &entries[entry_count];
    memset(e, 0, sizeof(*e));
    strncpy(e->name, name, DEVICE_NAME_MAX - 1);
    e->parent = parent;
    e->base = base;

    return entry_count++;
}
