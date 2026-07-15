#include <sra32/cpu.h>
#include <util/bit.h>
#include <util/log.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void cpu_init(cpu_t *cpu, bus_t *bus, const isa_t *isa)
{
    memset(cpu, 0, sizeof *cpu);
    cpu->bus = bus;
    cpu->isa = isa;
}

void cpu_reset(cpu_t *cpu, uint32_t entry)
{
    memset(cpu->regs, 0, sizeof cpu->regs);
    cpu->flags = 0;
    cpu->pc = entry;
    cpu->halted = false;
    cpu->halt_reason[0] = '\0';
}

uint32_t cpu_get_reg(const cpu_t *cpu, uint8_t index)
{
    return index == REG_X0 ? 0 : cpu->regs[index & 0x1F];
}

void cpu_set_reg(cpu_t *cpu, uint8_t index, uint32_t value)
{
    if (index != REG_X0)
        cpu->regs[index & 0x1F] = value;
}

bool cpu_get_flag(const cpu_t *cpu, flag_t f)
{
    return (cpu->flags & f) != 0;
}

void cpu_set_flag(cpu_t *cpu, flag_t f, bool on)
{
    if (on)
        cpu->flags |= f;
    else
        cpu->flags &= ~f;
}

void cpu_halt(cpu_t *cpu, const char *fmt, ...)
{
    if (cpu->halted)
        return;
    cpu->halted = true;

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(cpu->halt_reason, sizeof cpu->halt_reason, fmt, ap);
    va_end(ap);

    log_warn("halted: %s", cpu->halt_reason);
}

bool cpu_step(cpu_t *cpu)
{
    if (cpu->halted)
        return false;

    const uint32_t pc = cpu->pc;

    uint32_t word;
    if (!bus_read32(cpu->bus, pc, &word))
    {
        cpu_halt(cpu, "bus fault fetching instruction @ 0x%08x", pc);
        return false;
    }

    const decoded_t in = instr_decode(pc, word);
    cpu->pc = pc + 4;

    const isa_instr_t *instr = isa_find(cpu->isa, in.form, in.op);
    if (!instr)
    {
        cpu_halt(cpu, "unhandled instruction (form=%s, op=0x%02x) @ 0x%08x", form_name(in.form), in.op, in.pc);
        return false;
    }

    if (log_enabled(LOG_TRACE))
    {
        char buf[128];
        isa_disasm(cpu->isa, &in, buf, sizeof buf);
        log_trace("0x%08x: %s", in.pc, buf);
    }

    instr->exec(cpu, &in);
    return !cpu->halted;
}

void cpu_dump(const cpu_t *cpu, FILE *out)
{
    fprintf(out, "pc=0x%08x flags=0x%08x [z=%d n=%d c=%d ie=%d u=%d ptag=0x%04x]\n",
            cpu->pc, cpu->flags,
            cpu_get_flag(cpu, FLAG_ZERO),
            cpu_get_flag(cpu, FLAG_NEGATIVE),
            cpu_get_flag(cpu, FLAG_CARRY),
            cpu_get_flag(cpu, FLAG_INT_ENABLE),
            cpu_get_flag(cpu, FLAG_USER),
            BITS(cpu->flags, 16, 16));

    for (int i = 0; i < 32; i++)
        fprintf(out, "%-3s=0x%08x%s", reg_name((uint8_t)i), cpu->regs[i],
                i % 4 == 3 ? "\n" : "  ");
}

uint64_t cpu_run(cpu_t *cpu, uint64_t max_steps)
{
    uint64_t retired = 0;
    while ((max_steps == 0 || retired < max_steps) && cpu_step(cpu))
        retired++;
    return retired;
}
