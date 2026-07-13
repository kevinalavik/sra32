#ifndef DEVICE_H
#define DEVICE_H

#include <stdint.h>

#define DEVICE_NAME_MAX 16
#define DEVICE_MAX 64
#define DEVICE_NONE 0xFFFFFFFF
#define DEVICE_TREE_BASE 0xFFFE0000

void device_tree_init(void);
uint32_t device_tree_add(const char *name, uint32_t parent, uint32_t base);

#endif // DEVICE_H
