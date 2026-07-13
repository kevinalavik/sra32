#ifndef SRA32KA_DEVICE_H
#define SRA32KA_DEVICE_H

#include <stdint.h>

#define SRA32KA_DEVTREE_BASE 0xFFFE0000
#define SRA32KA_DEVTREE_ENTRY_SIZE 0x20
#define SRA32KA_DEVTREE_ENTRIES_OFFSET 0x20
#define SRA32KA_DEVICE_NAME_MAX 16
#define SRA32KA_DEVICE_NONE 0xFFFFFFFF

typedef struct
{
    uint32_t index;
    uint32_t base;
} sra32ka_device_t;

static inline uint32_t sra32ka_device_count(void)
{
    return *(volatile uint32_t *)(SRA32KA_DEVTREE_BASE);
}

static inline uint32_t sra32ka_entry_field(uint32_t index, uint32_t offset)
{
    return *(volatile uint32_t *)(SRA32KA_DEVTREE_BASE + SRA32KA_DEVTREE_ENTRIES_OFFSET +
                                  index * SRA32KA_DEVTREE_ENTRY_SIZE + offset);
}

static inline uint32_t sra32ka_entry_name_byte(uint32_t index, uint32_t i)
{
    uint32_t word_off = SRA32KA_DEVTREE_ENTRIES_OFFSET + index * SRA32KA_DEVTREE_ENTRY_SIZE + (i & ~3u);
    uint32_t word = *(volatile uint32_t *)(SRA32KA_DEVTREE_BASE + word_off);
    return (word >> (8 * (i & 3u))) & 0xFFu;
}

static inline int sra32ka_segment_matches(uint32_t index, const char *segment)
{
    uint32_t i = 0;

    while (i < SRA32KA_DEVICE_NAME_MAX)
    {
        uint32_t c = sra32ka_entry_name_byte(index, i);
        uint32_t n = (uint32_t)(unsigned char)segment[i];

        if (n == '.')
            n = 0;

        if (c != n)
            return 0;
        if (c == 0)
            return 1;
        i++;
    }
    return 0;
}

static inline uint32_t sra32ka_device_parent(const sra32ka_device_t *dev)
{
    return sra32ka_entry_field(dev->index, 0x10);
}

static inline int sra32ka_device_open(const char *path, sra32ka_device_t *dev)
{
    uint32_t count = sra32ka_device_count();
    uint32_t parent = SRA32KA_DEVICE_NONE;
    uint32_t found = SRA32KA_DEVICE_NONE;
    const char *segment = path;

    while (*segment)
    {
        uint32_t i;
        const char *next;

        found = SRA32KA_DEVICE_NONE;
        for (i = 0; i < count; i++)
        {
            sra32ka_device_t candidate;
            candidate.index = i;
            if (sra32ka_device_parent(&candidate) == parent && sra32ka_segment_matches(i, segment))
            {
                found = i;
                break;
            }
        }

        if (found == SRA32KA_DEVICE_NONE)
            return -1;

        parent = found;
        next = segment;
        while (*next && *next != '.')
            next++;
        segment = (*next == '.') ? next + 1 : next;
    }

    if (found == SRA32KA_DEVICE_NONE)
        return -1;

    dev->index = found;
    dev->base = sra32ka_entry_field(found, 0x14);
    return 0;
}

static inline uint32_t sra32ka_device_read32(const sra32ka_device_t *dev, uint32_t offset)
{
    return *(volatile uint32_t *)(dev->base + offset);
}

static inline void sra32ka_device_write32(const sra32ka_device_t *dev, uint32_t offset, uint32_t value)
{
    *(volatile uint32_t *)(dev->base + offset) = value;
}

#endif // SRA32KA_DEVICE_H
