#include "internal.h"

static void exec_sui(cpu_t *cpu, const decoded_t *in)
{
    cpu_set_reg(cpu, in->rd, cpu_get_reg(cpu, in->rd) | (in->imm << 12));
}

static void disasm_sui(const decoded_t *in, char *buf, size_t len)
{
    DISASM("sui %s, 0x%05x", reg_name(in->rd), in->imm);
}

const isa_instr_t isa_form_u[] = {
    {FORM_U, 0x0, "sui", exec_sui, disasm_sui},
};

const size_t isa_form_u_count = ARRAY_SIZE(isa_form_u);
