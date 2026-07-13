#ifndef SRA32KA_ASM_H
#define SRA32KA_ASM_H

#include <sra32/asm.h>

/* sra32ka specific instructions (along with simplified macro) */
#define SRA32KA_OP_HALT 0x20
#define SRA32KA_HALT() SRA32_EMIT(SRA32_ENC_R(SRA32KA_OP_HALT, 0, 0, 0, 0))

#endif // SRA32KA_ASM_H