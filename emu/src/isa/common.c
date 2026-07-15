#include "internal.h"

#include <util/bit.h>
#include <util/log.h>

static const char *const alu_functs[] = {
#define X(id, funct, name) [funct] = name,
    ALU_FUNCTS(X)
#undef X
};

const char *isa_alu_funct_name(uint8_t funct)
{
    return funct < ARRAY_SIZE(alu_functs) ? alu_functs[funct] : NULL;
}

enum
{
    COND_ALWAYS = 0,
    COND_EQ = 1,
    COND_NE = 2,
    COND_LT = 3,
    COND_GT = 4,
    COND_LE = 5,
    COND_GE = 6,
};

bool isa_cond_eval(const cpu_t *cpu, uint8_t cond)
{
    const bool z = cpu_get_flag(cpu, FLAG_ZERO);
    const bool lt = cpu_get_flag(cpu, (cond & BIT(3)) ? FLAG_CARRY : FLAG_NEGATIVE);

    switch (cond & 0x7)
    {
    case COND_ALWAYS:
        return true;
    case COND_EQ:
        return z;
    case COND_NE:
        return !z;
    case COND_LT:
        return lt;
    case COND_GT:
        return !lt && !z;
    case COND_LE:
        return lt || z;
    case COND_GE:
        return !lt;
    default:
        log_warn("Unknown condition: %d!", cond & 0x7);
        return false;
    }
}

const char *isa_cond_name(uint8_t cond)
{
    static const char *const names[16] = {
        "", "eq", "ne", "lt", "gt", "le", "ge", "?",
        "u", "equ", "neu", "ltu", "gtu", "leu", "geu", "?u"};
    return names[cond & 0xF];
}

static void set_zn(cpu_t *cpu, uint32_t value)
{
    cpu_set_flag(cpu, FLAG_ZERO, value == 0);
    cpu_set_flag(cpu, FLAG_NEGATIVE, (int32_t)value < 0);
}

bool isa_priv_check(cpu_t *cpu, const decoded_t *in, const char *what)
{
    if (!cpu_get_flag(cpu, FLAG_USER))
        return true;
    cpu_halt(cpu, "privileged instruction in user mode: %s @ pc=0x%08x", what, in->pc);
    return false;
}

bool isa_alu_eval(cpu_t *cpu, uint8_t funct, uint32_t t, uint32_t s, uint32_t *out)
{
    uint32_t r = 0;

    switch (funct)
    {
    case ALU_ADD:
    case ALU_ADDC:
    {
        const uint32_t c = funct == ALU_ADDC && cpu_get_flag(cpu, FLAG_CARRY);
        const uint64_t wide = (uint64_t)t + s + c;
        r = (uint32_t)wide;
        cpu_set_flag(cpu, FLAG_CARRY, (wide >> 32) != 0);
        set_zn(cpu, r);
        break;
    }
    case ALU_SUB:
    case ALU_SUBC:
    {
        const uint32_t b = funct == ALU_SUBC && cpu_get_flag(cpu, FLAG_CARRY);
        const uint64_t wide = (uint64_t)t - s - b;
        r = (uint32_t)wide;
        cpu_set_flag(cpu, FLAG_CARRY, (wide >> 32) != 0); /* borrow */
        set_zn(cpu, r);
        break;
    }
    case ALU_MUL:
    case ALU_UMUL:
        /* low 32 bits are identical for signed and unsigned multiply */
        r = t * s;
        break;
    /* division by zero: all ones, remainder = dividend (no trap for now) */
    case ALU_REM:
        if (s == 0)
            r = t;
        else if ((int32_t)t == INT32_MIN && (int32_t)s == -1)
            r = 0;
        else
            r = (uint32_t)((int32_t)t % (int32_t)s);
        break;
    case ALU_UREM:
        r = s == 0 ? t : t % s;
        break;
    case ALU_DIV:
        if (s == 0)
            r = UINT32_MAX;
        else if ((int32_t)t == INT32_MIN && (int32_t)s == -1)
            r = (uint32_t)INT32_MIN;
        else
            r = (uint32_t)((int32_t)t / (int32_t)s);
        break;
    case ALU_UDIV:
        r = s == 0 ? UINT32_MAX : t / s;
        break;
    case ALU_AND:
        r = t & s;
        break;
    case ALU_OR:
        r = t | s;
        break;
    case ALU_XOR:
        r = t ^ s;
        break;
    case ALU_SHL:
        r = t << (s & 31);
        break;
    case ALU_SHR:
        r = t >> (s & 31);
        break;
    case ALU_SAR:
        r = (uint32_t)((int32_t)t >> (s & 31));
        break;
    case ALU_NOT:
        r = ~t;
        break;
    default:
        log_warn("Unknown ALU funct: %d!", funct);
        return false;
    }

    *out = r;
    return true;
}

uint32_t isa_mem_size(uint8_t funct)
{
    return funct <= 2 ? 1u << funct : 0;
}

bool isa_mem_read(cpu_t *cpu, const decoded_t *in, uint32_t addr, uint32_t size, uint32_t *out)
{
    bool ok;
    if (size == 4)
    {
        ok = bus_read32(cpu->bus, addr, out);
    }
    else
    {
        uint8_t b[4] = {0};
        ok = true;
        for (uint32_t i = 0; i < size && ok; i++)
            ok = bus_read8(cpu->bus, addr + i, &b[i]);
        *out = LE32_LOAD(b);
    }

    if (!ok)
        cpu_halt(cpu, "bus fault reading %u byte(s) @ 0x%08x (pc=0x%08x)", size, addr, in->pc);
    return ok;
}

bool isa_mem_write(cpu_t *cpu, const decoded_t *in, uint32_t addr, uint32_t size, uint32_t value)
{
    bool ok;
    if (size == 4)
    {
        ok = bus_write32(cpu->bus, addr, value);
    }
    else
    {
        ok = true;
        for (uint32_t i = 0; i < size && ok; i++)
            ok = bus_write8(cpu->bus, addr + i, BYTE(value, i));
    }

    if (!ok)
        cpu_halt(cpu, "bus fault writing %u byte(s) @ 0x%08x (pc=0x%08x)", size, addr, in->pc);
    return ok;
}
