#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <sra32/bus.h>
#include <sra32/isa.h>

/*
 * Flags register layout:
 * [ ptag(16) | reserved(12) | user(1) | interrupt enable(1) | carry(1) | negative(1) | zero(1) ]
 */
typedef enum flag
{
    FLAG_ZERO = 1u << 0,
    FLAG_NEGATIVE = 1u << 1,
    FLAG_CARRY = 1u << 2,
    FLAG_INT_ENABLE = 1u << 3,
    FLAG_USER = 1u << 4,
} flag_t;

/* register indexes */
enum
{
    REG_X0 = 0, /* hardwired zero */
    REG_RP = 1, /* return pointer */
    REG_SP = 2, /* stack pointer */
    REG_FP = 3, /* frame pointer */
    REG_R0 = 4, /* r0-r27 follow */
};

/*
 * sra32 ABI
 * =========
 * - r0-r11  = caller saved
 * - r12-r27 = callee saved
 * - rp      = call clobbered
 */
struct cpu
{
    bus_t *bus;
    const isa_t *isa;
    uint32_t regs[32];
    uint32_t cr[16]; // special regs
    uint32_t pc;
    uint32_t flags;
    bool halted;
    char halt_reason[256];
};

void cpu_init(cpu_t *cpu, bus_t *bus, const isa_t *isa);
void cpu_reset(cpu_t *cpu, uint32_t entry);

uint32_t cpu_get_reg(const cpu_t *cpu, uint8_t index);
void cpu_set_reg(cpu_t *cpu, uint8_t index, uint32_t value);

bool cpu_get_flag(const cpu_t *cpu, flag_t f);
void cpu_set_flag(cpu_t *cpu, flag_t f, bool on);

void cpu_halt(cpu_t *cpu, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

void cpu_dump(const cpu_t *cpu, FILE *out);

bool cpu_step(cpu_t *cpu);
/* max_steps = 0 means run until halt */
uint64_t cpu_run(cpu_t *cpu, uint64_t max_steps);
