#ifndef SRA32_H
#define SRA32_H

#include <stdint.h>
#include <stddef.h>

/* main cpu context and registers */
enum {
    REG_X0, // always 0
    REG_RP, // return pointer
    REG_SP, // stack pointer
    REG_FP, // frame pinter
    REG_R0,
    REG_R1,
    REG_R2,
    REG_R3,
    REG_R4,
    REG_R5,
    REG_R6,
    REG_R7,
    REG_R8,
    REG_R9,
    REG_R10,
    REG_R11,
    REG_R12,
    REG_R13,
    REG_R14,
    REG_R15,
    REG_R16,
    REG_R17,
    REG_R18,
    REG_R19,
    REG_R20,
    REG_R21,
    REG_R22,
    REG_R23,
    REG_R24,
    REG_R25,
    REG_R26,
    REG_R27,
    REG_COUNT
};

enum {
    FLAG_Z  = 1 << 0,
    FLAG_N  = 1 << 1,
    FLAG_C  = 1 << 2,
    FLAG_O  = 1 << 3,
    FLAG_IE = 1 << 4
};

#define SRA32_FLAG_MASK 0x1F

/* emulator traps (non ISA, for now) */
enum {
    SRA32_TRAP_NONE = 0,
    SRA32_TRAP_ILLEGAL, // undefined opcode or funct
};

typedef struct cpu cpu_t;

typedef uint32_t (*sra32_read_fn)(cpu_t* cpu, uint32_t addr, uint32_t size);
typedef void (*sra32_write_fn)(cpu_t* cpu, uint32_t addr, uint32_t value, uint32_t size);
typedef void (*sra32_trap_fn)(cpu_t* cpu, uint32_t reason, uint32_t word);

struct cpu {
    uint32_t regs[REG_COUNT];
    uint32_t pc;
    uint32_t flags;
    int halted;
 
    sra32_read_fn read;
    sra32_write_fn write;
    sra32_trap_fn trap;
    void* user; // free for use in the bus implementation (or trap ig)
};

/* instruction definition and handeling */
#define INSTR_FORMAT_R 0x01 // Register format:  [ Opc (6) | RD (5) | RT (5) | RS (5) | Funct (4) | Unused (7) ]
#define INSTR_FORMAT_I 0x02 // Immediate format: [ Opc (6) | RD (5) | RT (5) | Funct (4) | Imm (12) ]
#define INSTR_FORMAT_U 0x03 // Upper format:     [ Opc (6) | RD (5) | Unused (1) | Imm (20) ]
#define INSTR_FORMAT_J 0x04 // Jump format:      [ Opc (6) | Funct (3) | Address (23) ]

typedef struct {
    uint32_t pc;   // address of this instruction (cpu->pc already points past it)
    uint32_t word; // raw encoding
    uint8_t op;
    uint8_t kind;  // INSTR_FORMAT_*
    uint8_t rd;
    uint8_t rt;
    uint8_t rs;
    uint8_t funct;
    uint32_t imm;  // raw immediate / address field
    int32_t simm;  // sign-extended immediate / address field
} sra32_instr_t;


typedef struct {
    char mnemonic[16];
    uint8_t op;
    uint8_t kind;
    void (*handler)(cpu_t* cpu, const sra32_instr_t* in);
} sra32_op_t;

extern sra32_op_t sra32_ops[64];


/* alu / branch funct codes */
enum {
    ALU_ADD,
    ALU_SUB,
    ALU_MUL,
    ALU_REM,
    ALU_SDIV,
    ALU_UDIV,
    ALU_AND,
    ALU_OR,
    ALU_XOR,
    ALU_SHL,
    ALU_SHR
};

enum {
    MEM_BYTE,
    MEM_HALF,
    MEM_WORD
};

enum {
    BR_AL, 
    BR_EQ, 
    BR_NE, 
    BR_LT, 
    BR_GT, 
    BR_LE, 
    BR_GE
};

/* core api */
void sra32_init(cpu_t* cpu, sra32_read_fn read, sra32_write_fn write, sra32_trap_fn trap, void* user);
void sra32_reset(cpu_t* cpu, uint32_t pc);
void sra32_decode(cpu_t* cpu, uint32_t word, sra32_instr_t* out);
void sra32_step(cpu_t* cpu); // fetch, decode and execute one instruction
uint64_t sra32_run(cpu_t* cpu, uint64_t max); // run until halted, returns steps taken
void sra32_register_op(uint8_t op, const char* mnemonic, uint8_t kind, void (*handler)(cpu_t*, const sra32_instr_t*)); // custom opcodes

/* builtin handlers */
extern void cpu_handle_nop(cpu_t*, const sra32_instr_t*);
extern void cpu_handle_alu(cpu_t*, const sra32_instr_t*);
extern void cpu_handle_mov(cpu_t*, const sra32_instr_t*);
extern void cpu_handle_cmp(cpu_t*, const sra32_instr_t*);
extern void cpu_handle_push(cpu_t*, const sra32_instr_t*);
extern void cpu_handle_pop(cpu_t*, const sra32_instr_t*);
extern void cpu_handle_rfl(cpu_t*, const sra32_instr_t*);
extern void cpu_handle_wfl(cpu_t*, const sra32_instr_t*);
extern void cpu_handle_spc(cpu_t*, const sra32_instr_t*);
extern void cpu_handle_calr(cpu_t*, const sra32_instr_t*);
extern void cpu_handle_alui(cpu_t*, const sra32_instr_t*);
extern void cpu_handle_load(cpu_t*, const sra32_instr_t*);
extern void cpu_handle_store(cpu_t*, const sra32_instr_t*);
// not implemented // extern void cpu_handle_rcr(cpu_t*, const sra32_instr_t*);
// not implemented // extern void cpu_handle_wcr(cpu_t*, const sra32_instr_t*);
extern void cpu_handle_sui(cpu_t*, const sra32_instr_t*);
extern void cpu_handle_br(cpu_t*, const sra32_instr_t*);
extern void cpu_handle_call(cpu_t*, const sra32_instr_t*);

/* encoding helpers (i hate bit manipulationb) */
#define SRA32_ENC_R(op, rd, rt, rs, funct) \
    ((((uint32_t)(op) & 0x3F) << 26) | (((uint32_t)(rd) & 0x1F) << 21) | \
     (((uint32_t)(rt) & 0x1F) << 16) | (((uint32_t)(rs) & 0x1F) << 11) | \
     (((uint32_t)(funct) & 0x0F) << 7))
#define SRA32_ENC_I(op, rd, rt, funct, imm) \
    ((((uint32_t)(op) & 0x3F) << 26) | (((uint32_t)(rd) & 0x1F) << 21) | \
     (((uint32_t)(rt) & 0x1F) << 16) | (((uint32_t)(funct) & 0x0F) << 12) | \
     ((uint32_t)(imm) & 0xFFF))
#define SRA32_ENC_U(op, rd, imm) \
    ((((uint32_t)(op) & 0x3F) << 26) | (((uint32_t)(rd) & 0x1F) << 21) | \
     ((uint32_t)(imm) & 0xFFFFF))
#define SRA32_ENC_J(op, funct, addr) \
    ((((uint32_t)(op) & 0x3F) << 26) | (((uint32_t)(funct) & 0x07) << 23) | \
     ((uint32_t)(addr) & 0x7FFFFF))

/* implementation, stb style brrrr :^)*/
#ifdef SRA32_IMPLEMENTATION

// sign extend the low n bits of v
static int32_t sra32_sx(uint32_t v, int n) {
    uint32_t m = 1 << (n - 1);
    return (int32_t)((v ^ m) - m);
}

// emulator trap
static void sra32_trap(cpu_t* cpu, uint32_t reason, uint32_t word) {
    if (cpu->trap) cpu->trap(cpu, reason, word);
    else cpu->halted = 1;
}

// flag helpers
static void sra32_set_zn(cpu_t* cpu, uint32_t r) {
    cpu->flags &= ~(uint32_t)(FLAG_Z | FLAG_N);
    if (r == 0) cpu->flags |= FLAG_Z;
    if (r & 0x80000000) cpu->flags |= FLAG_N;
}
 
static void sra32_set_flags_add(cpu_t* cpu, uint32_t a, uint32_t b, uint32_t r) {
    sra32_set_zn(cpu, r);
    cpu->flags &= ~(uint32_t)(FLAG_C | FLAG_O);
    if (r < a) cpu->flags |= FLAG_C; // unsigned carry out
    if (~(a ^ b) & (a ^ r) & 0x80000000) cpu->flags |= FLAG_O; // signed overflow
}
 
static void sra32_set_flags_sub(cpu_t* cpu, uint32_t a, uint32_t b, uint32_t r) {
    sra32_set_zn(cpu, r);
    cpu->flags &= ~(uint32_t)(FLAG_C | FLAG_O);
    if (a >= b) cpu->flags |= FLAG_C; // C = no borrow
    if ((a ^ b) & (a ^ r) & 0x80000000) cpu->flags |= FLAG_O;
}

// one handler for "alu" and "alui"
static uint32_t sra32_alu(cpu_t* cpu, uint8_t funct, uint32_t a, uint32_t b, const sra32_instr_t* in) {
    uint32_t ret = 0;
    switch(funct) {
        case ALU_ADD: ret = a + b; sra32_set_flags_add(cpu, a, b, ret); return ret;
        case ALU_SUB: ret = a - b; sra32_set_flags_sub(cpu, a, b, ret); return ret;
        case ALU_MUL: ret = a * b; break;
        case ALU_REM:
            if (b == 0) ret = a;
            else if (a == 0x80000000 && b == 0xFFFFFFFF) ret = 0;
            else ret = (uint32_t)((int32_t)a % (int32_t)b);
            break;
        case ALU_SDIV:
            if (b == 0) ret = 0xFFFFFFFF; // div by zero -> -1
            else if (a == 0x80000000 && b == 0xFFFFFFFF) ret = 0x80000000; // INT_MIN / -1 (i think)
            else ret = (uint32_t)((int32_t)a / (int32_t)b);
            break;
        case ALU_UDIV: ret = (b == 0) ? 0xFFFFFFFF : a / b; break;
        case ALU_AND: ret = a & b; break;
        case ALU_OR: ret = a | b; break;
        case ALU_XOR: ret = a ^ b; break;
        case ALU_SHL: ret = a << (b & 31); break;
        case ALU_SHR: ret = a >> (b & 31); break;
        default:
            sra32_trap(cpu, SRA32_TRAP_ILLEGAL, in->word);
            return 0;
    }
    sra32_set_zn(cpu, ret);
    return ret;
}

/* r form instrcution handlers */
void cpu_handle_nop(cpu_t* cpu, const sra32_instr_t* in) {
    (void)cpu; (void)in;
}
 
void cpu_handle_alu(cpu_t* cpu, const sra32_instr_t* in) {
    cpu->regs[in->rd] = sra32_alu(cpu, in->funct, cpu->regs[in->rt], cpu->regs[in->rs], in);
}
 
void cpu_handle_mov(cpu_t* cpu, const sra32_instr_t* in) {
    cpu->regs[in->rd] = cpu->regs[in->rs];
}
 
void cpu_handle_cmp(cpu_t* cpu, const sra32_instr_t* in) {
    uint32_t a = cpu->regs[in->rd], b = cpu->regs[in->rs];
    sra32_set_flags_sub(cpu, a, b, a - b);
}
 
void cpu_handle_push(cpu_t* cpu, const sra32_instr_t* in) {
    cpu->regs[REG_SP] -= 4;
    cpu->write(cpu, cpu->regs[REG_SP], cpu->regs[in->rd], 4);
}
 
void cpu_handle_pop(cpu_t* cpu, const sra32_instr_t* in) {
    cpu->regs[in->rd] = cpu->read(cpu, cpu->regs[REG_SP], 4);
    cpu->regs[REG_SP] += 4;
}
 
void cpu_handle_rfl(cpu_t* cpu, const sra32_instr_t* in) {
    cpu->regs[in->rd] = cpu->flags;
}
 
void cpu_handle_wfl(cpu_t* cpu, const sra32_instr_t* in) {
    cpu->flags = cpu->regs[in->rd] & SRA32_FLAG_MASK;
}
 
void cpu_handle_spc(cpu_t* cpu, const sra32_instr_t* in) {
    cpu->pc = cpu->regs[in->rd];
}
 
void cpu_handle_calr(cpu_t* cpu, const sra32_instr_t* in) {
    uint32_t target = cpu->regs[in->rd];
    cpu->regs[REG_RP] = in->pc + 4;
    cpu->pc = target;
}

/* i form instruction handlers */
void cpu_handle_alui(cpu_t* cpu, const sra32_instr_t* in) {
    cpu->regs[in->rd] = sra32_alu(cpu, in->funct, cpu->regs[in->rt], (uint32_t)in->simm, in);
}

void cpu_handle_load(cpu_t* cpu, const sra32_instr_t* in) {
    uint32_t addr = cpu->regs[in->rt] + (uint32_t)in->simm;
    switch(in->funct) {
        case MEM_BYTE: cpu->regs[in->rd] = cpu->read(cpu, addr, 1) & 0xFF; break;
        case MEM_HALF: cpu->regs[in->rd] = cpu->read(cpu, addr, 2) & 0xFFFF; break;
        case MEM_WORD: cpu->regs[in->rd] = cpu->read(cpu, addr, 4); break;
        default: sra32_trap(cpu, SRA32_TRAP_ILLEGAL, in->word); break;
    }
}

void cpu_handle_store(cpu_t* cpu, const sra32_instr_t* in) {
    uint32_t addr = cpu->regs[in->rt] + (uint32_t)in->simm;
    switch(in->funct) {
        case MEM_BYTE: cpu->write(cpu, addr, cpu->regs[in->rd], 1); break;
        case MEM_HALF: cpu->write(cpu, addr, cpu->regs[in->rd], 2); break;
        case MEM_WORD: cpu->write(cpu, addr, cpu->regs[in->rd], 4); break;
        default: sra32_trap(cpu, SRA32_TRAP_ILLEGAL, in->word); break;
    }
}

/* u form instruction handlers, single little opcode >:D */
void cpu_handle_sui(cpu_t* cpu, const sra32_instr_t* in) {
    cpu->regs[in->rd] = in->imm << 12;
}

/* j form handlers */
static int sra32_cond(const cpu_t* cpu, uint8_t funct) {
    uint32_t f = cpu->flags;
    int z = !!(f & FLAG_Z), n = !!(f & FLAG_N), v = !!(f & FLAG_O);
    switch (funct) {
        case BR_AL: return 1;
        case BR_EQ: return z;
        case BR_NE: return !z;
        case BR_LT: return n != v; // signed <
        case BR_GT: return !z && n == v; // signed >
        case BR_LE: return z || n != v; // signed <=
        case BR_GE: return n == v; // signed >=
        default: return -1;
    }
}

void cpu_handle_br(cpu_t* cpu, const sra32_instr_t* in) {
    int taken = sra32_cond(cpu, in->funct);
    if (taken < 0) { sra32_trap(cpu, SRA32_TRAP_ILLEGAL, in->word); return; }
    if (taken)
        cpu->pc = in->pc + (uint32_t)in->simm;
}
 
void cpu_handle_call(cpu_t* cpu, const sra32_instr_t* in) {
    cpu->regs[REG_RP] = in->pc + 4;
    cpu->pc = in->pc + (uint32_t)in->simm;
}

/* the opcode table */
sra32_op_t sra32_ops[64] = {
    [0x00] = {"nop",   0x00, INSTR_FORMAT_R, cpu_handle_nop},
    [0x01] = {"alu",   0x01, INSTR_FORMAT_R, cpu_handle_alu},
    [0x02] = {"mov",   0x02, INSTR_FORMAT_R, cpu_handle_mov},
    [0x03] = {"cmp",   0x03, INSTR_FORMAT_R, cpu_handle_cmp},
    [0x04] = {"push",  0x04, INSTR_FORMAT_R, cpu_handle_push},
    [0x05] = {"pop",   0x05, INSTR_FORMAT_R, cpu_handle_pop},
    [0x06] = {"rfl",   0x06, INSTR_FORMAT_R, cpu_handle_rfl},
    [0x07] = {"wfl",   0x07, INSTR_FORMAT_R, cpu_handle_wfl},
    [0x08] = {"spc",   0x08, INSTR_FORMAT_R, cpu_handle_spc},
    [0x09] = {"calr",  0x09, INSTR_FORMAT_R, cpu_handle_calr},
    [0x0A] = {"alui",  0x0A, INSTR_FORMAT_I, cpu_handle_alui},
    [0x0B] = {"load",  0x0B, INSTR_FORMAT_I, cpu_handle_load},
    [0x0C] = {"store", 0x0C, INSTR_FORMAT_I, cpu_handle_store},
    [0x0D] = {"rcr",   0x0D, INSTR_FORMAT_I, NULL},// not implemented //
    [0x0E] = {"wcr",   0x0E, INSTR_FORMAT_I, NULL},// not implemented //
    [0x0F] = {"sui",   0x0F, INSTR_FORMAT_U, cpu_handle_sui},
    [0x10] = {"br",    0x10, INSTR_FORMAT_J, cpu_handle_br},
    [0x11] = {"call",  0x11, INSTR_FORMAT_J, cpu_handle_call},
};

/* core api */
void sra32_init(cpu_t* cpu, sra32_read_fn read, sra32_write_fn write, sra32_trap_fn trap, void* user) {
    for (size_t i = 0; i < REG_COUNT; i++) cpu->regs[i] = 0;
    cpu->pc = 0;
    cpu->flags = 0;
    cpu->halted = 0;
    cpu->read = read;
    cpu->write = write;
    cpu->trap = trap;
    cpu->user = user;
}

void sra32_reset(cpu_t* cpu, uint32_t pc) {
    for (size_t i = 0; i < REG_COUNT; i++) cpu->regs[i] = 0;
    cpu->pc = pc;
    cpu->flags = 0;
    cpu->halted = 0;
}

void sra32_decode(cpu_t* cpu, uint32_t word, sra32_instr_t* out) {
    out->pc = cpu->pc;
    out->word = word;
    out->op = (uint8_t)(word >> 26);
    out->kind = sra32_ops[out->op].kind;
    out->imm = 0;
    out->simm = 0;

    switch (out->kind) {
        case INSTR_FORMAT_R:
            out->rd = (word >> 21) & 0x1F;
            out->rt = (word >> 16) & 0x1F;
            out->rs = (word >> 11) & 0x1F;
            out->funct = (word >> 7) & 0x0F;
            break;
        case INSTR_FORMAT_I:
            out->rd = (word >> 21) & 0x1F;
            out->rt = (word >> 16) & 0x1F;
            out->funct = (word >> 12) & 0x0F;
            out->imm = word & 0xFFF;
            out->simm = sra32_sx(out->imm, 12);
            break;
        case INSTR_FORMAT_U:
            out->rd = (word >> 21) & 0x1F;
            out->imm = word & 0xFFFFF;
            out->simm = sra32_sx(out->imm, 20);
            break;
        case INSTR_FORMAT_J:
            out->funct = (word >> 23) & 0x07;
            out->imm = word & 0x7FFFFF;
            out->simm = sra32_sx(out->imm, 23);
            break;
    }
}

void sra32_step(cpu_t* cpu) {
    sra32_instr_t in;
    uint32_t word;
 
    if (cpu->halted)
        return;
 
    word = cpu->read(cpu, cpu->pc, 4);
    sra32_decode(cpu, word, &in);
    cpu->pc += 4;
 
    if (sra32_ops[in.op].handler == NULL) {
        sra32_trap(cpu, SRA32_TRAP_ILLEGAL, word);
        return;
    }
 
    sra32_ops[in.op].handler(cpu, &in);
    cpu->regs[REG_X0] = 0;
}

uint64_t sra32_run(cpu_t* cpu, uint64_t max) {
    uint64_t steps = 0;
    while (!cpu->halted && (max == 0 || steps < max)) {
        sra32_step(cpu);
        steps++;
    }
    return steps;
}

void sra32_register_op(uint8_t op, const char* mnemonic, uint8_t kind,
                       void (*handler)(cpu_t*, const sra32_instr_t*)) {
    size_t i;
    sra32_op_t* e = &sra32_ops[op & 0x3F];
    for (i = 0; i < sizeof(e->mnemonic) - 1 && mnemonic[i]; i++)
        e->mnemonic[i] = mnemonic[i];
    e->mnemonic[i] = '\0';
    e->op = op & 0x3F;
    e->kind = kind;
    e->handler = handler;
}

#endif // SRA32_IMPLEMENTATION
#endif // SRA32_H