#include "internal.h"

static void exec_br(cpu_t *cpu, const decoded_t *in)
{
    if (isa_cond_eval(cpu, in->funct))
        cpu->pc = in->pc + (uint32_t)instr_simm(in);
}

static void disasm_br(const decoded_t *in, char *buf, size_t len)
{
    const char *cn = isa_cond_name(in->funct);
    const uint32_t target = in->pc + (uint32_t)instr_simm(in);
    if (cn[0])
        DISASM("br.%s 0x%08x", cn, target);
    else
        DISASM("br 0x%08x", target);
}

static void exec_call(cpu_t *cpu, const decoded_t *in)
{
    cpu_set_reg(cpu, REG_RP, in->pc + 4);
    cpu->pc = in->pc + (uint32_t)instr_simm(in);
}

static void disasm_call(const decoded_t *in, char *buf, size_t len)
{
    DISASM("call 0x%08x", in->pc + (uint32_t)instr_simm(in));
}

const isa_instr_t isa_form_j[] = {
    {FORM_J, 0x0, "br", exec_br, disasm_br},
    {FORM_J, 0x1, "call", exec_call, disasm_call},
};

const size_t isa_form_j_count = ARRAY_SIZE(isa_form_j);
