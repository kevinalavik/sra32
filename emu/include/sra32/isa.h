#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <sra32/instr.h>

typedef struct cpu cpu_t;

typedef struct isa_instr
{
    form_t form;
    uint8_t op;
    const char *mnemonic;
    void (*exec)(cpu_t *cpu, const decoded_t *in);
    void (*disasm)(const decoded_t *in, char *buf, size_t len);
} isa_instr_t;

#define ISA_MAX_OPS 16

typedef struct isa
{
    const isa_instr_t *table[4 * ISA_MAX_OPS];
} isa_t;

bool isa_init(isa_t *isa);
bool isa_add(isa_t *isa, const isa_instr_t *instr);
const isa_instr_t *isa_find(const isa_t *isa, form_t form, uint8_t op);
void isa_disasm(const isa_t *isa, const decoded_t *in, char *buf, size_t len);
