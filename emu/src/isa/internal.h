#pragma once

#include <sra32/isa.h>
#include <sra32/cpu.h>

#include <stdio.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define DISASM(...) snprintf(buf, len, __VA_ARGS__)

/*
 * X(enum id, funct, mnemonic)
 */
#define ALU_FUNCTS(X)      \
    X(ALU_ADD, 0, "add")   \
    X(ALU_ADDC, 1, "addc") \
    X(ALU_SUB, 2, "sub")   \
    X(ALU_SUBC, 3, "subc") \
    X(ALU_MUL, 4, "mul")   \
    X(ALU_UMUL, 5, "umul") \
    X(ALU_REM, 6, "rem")   \
    X(ALU_UREM, 7, "urem") \
    X(ALU_DIV, 8, "div")   \
    X(ALU_UDIV, 9, "udiv") \
    X(ALU_AND, 10, "and")  \
    X(ALU_OR, 11, "or")    \
    X(ALU_XOR, 12, "xor")  \
    X(ALU_SHL, 13, "shl")  \
    X(ALU_SHR, 14, "shr")  \
    X(ALU_SAR, 15, "sar")  \
    X(ALU_NOT, 16, "not")

enum
{
#define X(id, funct, name) id = funct,
    ALU_FUNCTS(X)
#undef X
};

const char *isa_alu_funct_name(uint8_t funct);

/*
 * ALU core shared by alu, alui and cmp.
 * add/addc/sub/subc update Z/N/C (carry acts as borrow for subtraction);
 * the remaining ops leave flags untouched.
 * Returns false for an unknown funct (nothing written to *out).
 */
bool isa_alu_eval(cpu_t *cpu, uint8_t funct, uint32_t t, uint32_t s, uint32_t *out);
bool isa_cond_eval(const cpu_t *cpu, uint8_t cond);
const char *isa_cond_name(uint8_t cond);

/* halts the cpu when a restricted instruction runs in user mode */
bool isa_priv_check(cpu_t *cpu, const decoded_t *in, const char *what);

uint32_t isa_mem_size(uint8_t funct);
bool isa_mem_read(cpu_t *cpu, const decoded_t *in, uint32_t addr, uint32_t size, uint32_t *out);
bool isa_mem_write(cpu_t *cpu, const decoded_t *in, uint32_t addr, uint32_t size, uint32_t value);

extern const isa_instr_t isa_form_r[];
extern const size_t isa_form_r_count;
extern const isa_instr_t isa_form_i[];
extern const size_t isa_form_i_count;
extern const isa_instr_t isa_form_u[];
extern const size_t isa_form_u_count;
extern const isa_instr_t isa_form_j[];
extern const size_t isa_form_j_count;
