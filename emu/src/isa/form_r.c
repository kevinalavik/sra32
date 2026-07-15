#include "internal.h"

#include <util/bit.h>
#include <util/log.h>

static void exec_nop(cpu_t *cpu, const decoded_t *in)
{
    (void)cpu;
    (void)in;
}

static void exec_alu(cpu_t *cpu, const decoded_t *in)
{
    uint32_t r;
    if (isa_alu_eval(cpu, in->funct, cpu_get_reg(cpu, in->rt),
                     cpu_get_reg(cpu, in->rs), &r))
        cpu_set_reg(cpu, in->rd, r);
}

static void disasm_alu(const decoded_t *in, char *buf, size_t len)
{
    const char *name = isa_alu_funct_name(in->funct);
    if (!name)
    {
        DISASM("alu.%u %s, %s, %s", in->funct,
               reg_name(in->rd), reg_name(in->rt), reg_name(in->rs));
        return;
    }

    /* not is unary: D = ~T, no source operand */
    if (in->funct == ALU_NOT)
        DISASM("alu.not %s, %s", reg_name(in->rd), reg_name(in->rt));
    else
        DISASM("alu.%s %s, %s, %s", name,
               reg_name(in->rd), reg_name(in->rt), reg_name(in->rs));
}

static void exec_sext(cpu_t *cpu, const decoded_t *in)
{
    const uint32_t value = cpu_get_reg(cpu, in->rs);

    switch (in->funct)
    {
    case 0: /* byte */
        cpu_set_reg(cpu, in->rd, (uint32_t)SEXT(value, 8));
        break;
    case 1: /* half */
        cpu_set_reg(cpu, in->rd, (uint32_t)SEXT(value, 16));
        break;
    default:
        log_warn("Unknown sext funct: %d!", in->funct);
        break;
    }
}

static void disasm_sext(const decoded_t *in, char *buf, size_t len)
{
    static const char *const suffix[] = {"b", "h"};
    if (in->funct >= ARRAY_SIZE(suffix))
        DISASM("sext.%u %s, %s", in->funct, reg_name(in->rd), reg_name(in->rs));
    else
        DISASM("sext.%s %s, %s", suffix[in->funct], reg_name(in->rd), reg_name(in->rs));
}

static void exec_mov(cpu_t *cpu, const decoded_t *in)
{
    /* funct MSB set means conditional move, low bits hold the condition */
    if ((in->funct & BIT(4)) && !isa_cond_eval(cpu, in->funct & 0xF))
        return;
    cpu_set_reg(cpu, in->rd, cpu_get_reg(cpu, in->rs));
}

static void disasm_mov(const decoded_t *in, char *buf, size_t len)
{
    const char *cn = (in->funct & BIT(4)) ? isa_cond_name(in->funct & 0xF) : "";
    if (cn[0])
        DISASM("mov.%s %s, %s", cn, reg_name(in->rd), reg_name(in->rs));
    else
        DISASM("mov %s, %s", reg_name(in->rd), reg_name(in->rs));
}

static void exec_cmp(cpu_t *cpu, const decoded_t *in)
{
    uint32_t r;
    isa_alu_eval(cpu, ALU_SUB, cpu_get_reg(cpu, in->rd), cpu_get_reg(cpu, in->rs), &r);
}

static void disasm_cmp(const decoded_t *in, char *buf, size_t len)
{
    DISASM("cmp %s, %s", reg_name(in->rd), reg_name(in->rs));
}

static void exec_push_pop(cpu_t *cpu, const decoded_t *in)
{
    const uint32_t sp = cpu_get_reg(cpu, REG_SP);

    switch (in->funct)
    {
    case 0: /* push: SP - 4; Memory[SP] = Rd */
        if (isa_mem_write(cpu, in, sp - 4, 4, cpu_get_reg(cpu, in->rd)))
            cpu_set_reg(cpu, REG_SP, sp - 4);
        break;
    case 1: /* pop: Rd = Memory[SP]; SP + 4 */
    {
        uint32_t value;
        if (isa_mem_read(cpu, in, sp, 4, &value))
        {
            cpu_set_reg(cpu, REG_SP, sp + 4);
            cpu_set_reg(cpu, in->rd, value);
        }
        break;
    }
    default:
        log_warn("Unknown push/pop funct: %d!", in->funct);
        break;
    }
}

static void disasm_push_pop(const decoded_t *in, char *buf, size_t len)
{
    DISASM("%s %s", in->funct == 0 ? "push" : "pop", reg_name(in->rd));
}

static void exec_rfl_wfl(cpu_t *cpu, const decoded_t *in)
{
    if (!isa_priv_check(cpu, in, "rfl/wfl"))
        return;

    switch (in->funct)
    {
    case 0: /* rfl: Rd = Flags */
        cpu_set_reg(cpu, in->rd, cpu->flags);
        break;
    case 1: /* wfl: Flags = Rd */
        cpu->flags = cpu_get_reg(cpu, in->rd);
        break;
    default:
        log_warn("Unknown rfl/wfl funct: %d!", in->funct);
        break;
    }
}

static void disasm_rfl_wfl(const decoded_t *in, char *buf, size_t len)
{
    DISASM("%s %s", in->funct == 0 ? "rfl" : "wfl", reg_name(in->rd));
}

static void exec_spc(cpu_t *cpu, const decoded_t *in)
{
    cpu->pc = cpu_get_reg(cpu, in->rd);
}

static void disasm_spc(const decoded_t *in, char *buf, size_t len)
{
    DISASM("spc %s", reg_name(in->rd));
}

static void exec_calr(cpu_t *cpu, const decoded_t *in)
{
    const uint32_t target = cpu_get_reg(cpu, in->rd);
    cpu_set_reg(cpu, REG_RP, in->pc + 4);
    cpu->pc = target;
}

static void disasm_calr(const decoded_t *in, char *buf, size_t len)
{
    DISASM("calr %s", reg_name(in->rd));
}

/* sysc, iret and fence are NOPs for now */

const isa_instr_t isa_form_r[] = {
    {FORM_R, 0x0, "nop", exec_nop, NULL},
    {FORM_R, 0x1, "alu", exec_alu, disasm_alu},
    {FORM_R, 0x2, "sext", exec_sext, disasm_sext},
    {FORM_R, 0x3, "mov", exec_mov, disasm_mov},
    {FORM_R, 0x4, "cmp", exec_cmp, disasm_cmp},
    {FORM_R, 0x5, "push/pop", exec_push_pop, disasm_push_pop},
    {FORM_R, 0x6, "rfl/wfl", exec_rfl_wfl, disasm_rfl_wfl},
    {FORM_R, 0x7, "spc", exec_spc, disasm_spc},
    {FORM_R, 0x8, "calr", exec_calr, disasm_calr},
    {FORM_R, 0x9, "sysc", exec_nop, NULL},
    {FORM_R, 0xa, "iret", exec_nop, NULL},
    {FORM_R, 0xb, "fence", exec_nop, NULL},
};

const size_t isa_form_r_count = ARRAY_SIZE(isa_form_r);
