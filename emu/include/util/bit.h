#pragma once

#include <stdint.h>

#define BIT(n) (1u << (n))
#define BIT_MASK(count) ((count) >= 32 ? 0xFFFFFFFFu : (1u << (count)) - 1u)
#define BITS(value, pos, count) (((uint32_t)(value) >> (pos)) & BIT_MASK(count))
#define SEXT(value, count) \
    ((int32_t)((uint32_t)(value) << (32 - (count))) >> (32 - (count)))
#define BYTE(value, n) ((uint8_t)((uint32_t)(value) >> ((n) * 8)))
#define LE32_LOAD(p) \
    ((uint32_t)(p)[0] | (uint32_t)(p)[1] << 8 | (uint32_t)(p)[2] << 16 | (uint32_t)(p)[3] << 24)
#define LE32_STORE(p, value)       \
    do                             \
    {                              \
        (p)[0] = BYTE((value), 0); \
        (p)[1] = BYTE((value), 1); \
        (p)[2] = BYTE((value), 2); \
        (p)[3] = BYTE((value), 3); \
    } while (0)
