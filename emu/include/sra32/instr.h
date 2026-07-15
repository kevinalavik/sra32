#pragma once

#include <stdint.h>

typedef enum form
{
    FORM_R = 0,
    FORM_I = 1,
    FORM_U = 2,
    FORM_J = 3,
} form_t;

const char *form_name(form_t form);
const char *reg_name(uint8_t index);

/*
 * R-Form: [ hint(2) | opc(4) | rd(5) | rt(5) | rs(5) | funct(5) | unused(6) ]
 * I-Form: [ hint(2) | opc(3) | rd(5) | rt(5) | funct(5) | imm(12) ]
 * U-Form: [ hint(2) | rd(5) | unused(5) | imm(20) ]
 * J-Form: [ hint(2) | opc(2) | funct(4) | address(24) ]
 */
typedef struct decoded
{
    uint32_t pc;
    uint32_t word;
    form_t form;
    uint8_t op;
    uint8_t rd;
    uint8_t rt;
    uint8_t rs;
    uint8_t funct;
    uint32_t imm;
} decoded_t;

decoded_t instr_decode(uint32_t pc, uint32_t word);

/* imm sign-extended from its natural width (12 for I, 24 for J) */
int32_t instr_simm(const decoded_t *in);
