#include "internal.h"
#include <util/log.h>
#include <string.h>

static size_t isa_key(form_t form, uint8_t op)
{
    return (size_t)form * ISA_MAX_OPS + op;
}

bool isa_add(isa_t *isa, const isa_instr_t *instr)
{
    if (instr->op >= ISA_MAX_OPS)
    {
        log_error("opcode out of range: form=%s op=%u", form_name(instr->form), instr->op);
        return false;
    }

    const isa_instr_t **slot = &isa->table[isa_key(instr->form, instr->op)];
    if (*slot)
    {
        log_error("duplicate instruction registration: form=%s op=%u (%s vs %s)", form_name(instr->form), instr->op, (*slot)->mnemonic, instr->mnemonic);
        return false;
    }

    *slot = instr;
    return true;
}

const isa_instr_t *isa_find(const isa_t *isa, form_t form, uint8_t op)
{
    if (op >= ISA_MAX_OPS)
        return NULL;
    return isa->table[isa_key(form, op)];
}

bool isa_init(isa_t *isa)
{
    static const struct
    {
        const isa_instr_t *instrs;
        const size_t *count;
    } forms[] = {
        {isa_form_r, &isa_form_r_count},
        {isa_form_i, &isa_form_i_count},
        {isa_form_u, &isa_form_u_count},
        {isa_form_j, &isa_form_j_count},
    };

    memset(isa, 0, sizeof *isa);

    for (size_t f = 0; f < ARRAY_SIZE(forms); f++)
        for (size_t i = 0; i < *forms[f].count; i++)
            if (!isa_add(isa, &forms[f].instrs[i]))
                return false;
    return true;
}

void isa_disasm(const isa_t *isa, const decoded_t *in, char *buf, size_t len)
{
    const isa_instr_t *instr = isa_find(isa, in->form, in->op);
    if (!instr)
    {
        snprintf(buf, len, ".word 0x%08x", in->word);
        return;
    }

    if (instr->disasm)
        instr->disasm(in, buf, len);
    else
        snprintf(buf, len, "%s", instr->mnemonic);
}
