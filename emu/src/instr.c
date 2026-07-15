#include <sra32/instr.h>
#include <util/bit.h>

const char *form_name(form_t form)
{
    switch (form)
    {
    case FORM_R:
        return "R";
    case FORM_I:
        return "I";
    case FORM_U:
        return "U";
    case FORM_J:
        return "J";
    }
    return "?";
}

const char *reg_name(uint8_t index)
{
    static const char *const names[32] = {
        "x0", "rp", "sp", "fp",
        "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
        "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
        "r24", "r25", "r26", "r27"};
    return index < 32 ? names[index] : "??";
}

decoded_t instr_decode(uint32_t pc, uint32_t word)
{
    decoded_t in = {0};
    in.pc = pc;
    in.word = word;
    in.form = (form_t)BITS(word, 30, 2);

    switch (in.form)
    {
    case FORM_R:
        in.op = BITS(word, 26, 4);
        in.rd = BITS(word, 21, 5);
        in.rt = BITS(word, 16, 5);
        in.rs = BITS(word, 11, 5);
        in.funct = BITS(word, 6, 5);
        break;
    case FORM_I:
        in.op = BITS(word, 27, 3);
        in.rd = BITS(word, 22, 5);
        in.rt = BITS(word, 17, 5);
        in.funct = BITS(word, 12, 5);
        in.imm = BITS(word, 0, 12);
        break;
    case FORM_U:
        in.rd = BITS(word, 25, 5);
        in.imm = BITS(word, 0, 20);
        break;
    case FORM_J:
        in.op = BITS(word, 28, 2);
        in.funct = BITS(word, 24, 4);
        in.imm = BITS(word, 0, 24);
        break;
    }

    return in;
}

int32_t instr_simm(const decoded_t *in)
{
    switch (in->form)
    {
    case FORM_I: /* 12-bit immediate */
        return SEXT(in->imm, 12);
    case FORM_J: /* 24-bit address */
        return SEXT(in->imm, 24);
    default:
        return (int32_t)in->imm;
    }
}
