#include "internal.h"

#include <util/log.h>

static void exec_alui(cpu_t *cpu, const decoded_t *in)
{
    uint32_t r;
    if (isa_alu_eval(cpu, in->funct, cpu_get_reg(cpu, in->rt), in->imm, &r))
        cpu_set_reg(cpu, in->rd, r);
}

static void disasm_alui(const decoded_t *in, char *buf, size_t len)
{
    const char *name = isa_alu_funct_name(in->funct);
    if (!name)
    {
        DISASM("alui.%u %s, %s, %u", in->funct, reg_name(in->rd), reg_name(in->rt), in->imm);
        return;
    }

    if (in->funct == ALU_NOT)
        DISASM("alui.not %s, %s", reg_name(in->rd), reg_name(in->rt));
    else
        DISASM("alui.%s %s, %s, %u", name, reg_name(in->rd), reg_name(in->rt), in->imm);
}

static void exec_rcr_wcr(cpu_t *cpu, const decoded_t *in)
{
    if (!isa_priv_check(cpu, in, "rcr/wcr"))
        return;

    if (in->imm >= ARRAY_SIZE(cpu->cr))
    {
        cpu_halt(cpu, "invalid control register %u @ pc=0x%08x", in->imm, in->pc);
        return;
    }

    switch (in->funct)
    {
    case 0: /* rcr: Rd = Cr[Imm] */
        cpu_set_reg(cpu, in->rd, cpu->cr[in->imm]);
        break;
    case 1: /* wcr: Cr[Imm] = Rd */
        cpu->cr[in->imm] = cpu_get_reg(cpu, in->rd);
        break;
    default:
        log_warn("Unknown rcr/wcr funct: %d!", in->funct);
        break;
    }
}

static void disasm_rcr_wcr(const decoded_t *in, char *buf, size_t len)
{
    DISASM("%s %s, %u", in->funct == 0 ? "rcr" : "wcr",
           reg_name(in->rd), in->imm);
}

static void exec_load(cpu_t *cpu, const decoded_t *in)
{
    const uint32_t size = isa_mem_size(in->funct);
    if (!size)
    {
        log_warn("Unknown load funct: %d!", in->funct);
        return;
    }

    const uint32_t addr = cpu_get_reg(cpu, in->rt) + (uint32_t)instr_simm(in);
    uint32_t value;
    if (isa_mem_read(cpu, in, addr, size, &value))
        cpu_set_reg(cpu, in->rd, value);
}

static void exec_store(cpu_t *cpu, const decoded_t *in)
{
    const uint32_t size = isa_mem_size(in->funct);
    if (!size)
    {
        log_warn("Unknown store funct: %d!", in->funct);
        return;
    }

    const uint32_t addr = cpu_get_reg(cpu, in->rt) + (uint32_t)instr_simm(in);
    isa_mem_write(cpu, in, addr, size, cpu_get_reg(cpu, in->rd));
}

static void disasm_mem(const decoded_t *in, char *buf, size_t len, const char *const mnemonics[3], const char *fallback)
{
    if (in->funct > 2)
        DISASM("%s.%u %s, %d[%s]", fallback, in->funct,
               reg_name(in->rd), instr_simm(in), reg_name(in->rt));
    else
        DISASM("%s %s, %d[%s]", mnemonics[in->funct],
               reg_name(in->rd), instr_simm(in), reg_name(in->rt));
}

static void disasm_load(const decoded_t *in, char *buf, size_t len)
{
    static const char *const mnemonics[3] = {"lb", "lh", "lw"};
    disasm_mem(in, buf, len, mnemonics, "load");
}

static void disasm_store(const decoded_t *in, char *buf, size_t len)
{
    static const char *const mnemonics[3] = {"sb", "sh", "sw"};
    disasm_mem(in, buf, len, mnemonics, "store");
}

static void exec_load_link(cpu_t *cpu, const decoded_t *in)
{
    /* single core: ll behaves as a plain word load */
    const uint32_t addr = cpu_get_reg(cpu, in->rt) + (uint32_t)instr_simm(in);
    uint32_t value;
    if (isa_mem_read(cpu, in, addr, 4, &value))
        cpu_set_reg(cpu, in->rd, value);
}

static void exec_store_conditional(cpu_t *cpu, const decoded_t *in)
{
    /* single core: sc always succeeds, plain word store */
    const uint32_t addr = cpu_get_reg(cpu, in->rt) + (uint32_t)instr_simm(in);
    isa_mem_write(cpu, in, addr, 4, cpu_get_reg(cpu, in->rd));
}

static void disasm_load_link(const decoded_t *in, char *buf, size_t len)
{
    DISASM("ll %s, %d[%s]", reg_name(in->rd), instr_simm(in), reg_name(in->rt));
}

static void disasm_store_conditional(const decoded_t *in, char *buf, size_t len)
{
    DISASM("sc %s, %d[%s]", reg_name(in->rd), instr_simm(in), reg_name(in->rt));
}

const isa_instr_t isa_form_i[] = {
    {FORM_I, 0x0, "alui", exec_alui, disasm_alui},
    {FORM_I, 0x1, "rcr/wcr", exec_rcr_wcr, disasm_rcr_wcr},
    {FORM_I, 0x2, "load", exec_load, disasm_load},
    {FORM_I, 0x3, "store", exec_store, disasm_store},
    {FORM_I, 0x4, "ll", exec_load_link, disasm_load_link},
    {FORM_I, 0x5, "sc", exec_store_conditional, disasm_store_conditional},
};

const size_t isa_form_i_count = ARRAY_SIZE(isa_form_i);
