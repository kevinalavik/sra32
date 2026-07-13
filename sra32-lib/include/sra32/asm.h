#ifndef SRA32_ASM_H
#define SRA32_ASM_H

#define SRA32_ENC_R(op, rd, rt, rs, funct) \
    ((((uint32_t)(op) & 0x3F) << 26) |     \
     (((uint32_t)(rd) & 0x1F) << 21) |     \
     (((uint32_t)(rt) & 0x1F) << 16) |     \
     (((uint32_t)(rs) & 0x1F) << 11) |     \
     (((uint32_t)(funct) & 0x0F) << 7))

#define SRA32_ENC_I(op, rd, rt, funct, imm) \
    ((((uint32_t)(op) & 0x3F) << 26) |      \
     (((uint32_t)(rd) & 0x1F) << 21) |      \
     (((uint32_t)(rt) & 0x1F) << 16) |      \
     (((uint32_t)(funct) & 0x0F) << 12) |   \
     ((uint32_t)(imm) & 0xFFF))

#define SRA32_ENC_U(op, rd, imm)       \
    ((((uint32_t)(op) & 0x3F) << 26) | \
     (((uint32_t)(rd) & 0x1F) << 21) | \
     ((uint32_t)(imm) & 0xFFFFF))

#define SRA32_ENC_J(op, funct, addr)      \
    ((((uint32_t)(op) & 0x3F) << 26) |    \
     (((uint32_t)(funct) & 0x07) << 23) | \
     ((uint32_t)(addr) & 0x7FFFFF))

#define SRA32_EMIT(word) asm volatile(".long %0\n" ::"i"(word))

#endif // SRA32_ASM_H