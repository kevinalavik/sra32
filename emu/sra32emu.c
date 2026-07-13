#define SRA32_IMPLEMENTATION
#include "sra32.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#define PROGRAM_NAME "sra32emu"
#define PROGRAM_VERSION "1.0"
#define PROGRAM_AUTHOR "Kevin Alavik"

#define RAM_SIZE_MAX 0x100000000
#define RAM_SIZE_DEFAULT RAM_SIZE_MAX // dirty but idfc

static uint8_t *ram;
static uint64_t ram_size = RAM_SIZE_DEFAULT;

static void cleanup(void)
{
    free(ram);
    ram = NULL;
}

/* cli options */
static struct
{
    int step;      // -s, interactive single stepping
    int dump_step; // -d, dump cpu state after every step
    int dump_end;  // -D, dump cpu state when execution finishes
    int disasm;    // -x, disassemble every instruction during runtime
    int elf;       // -e, treat the rom as an ELF executable (experimental)
    uint64_t max;  // -m, step limit (0 = unlimited)
} opts;

/* parse a size like 4096, 0x10000, 64K, 8M or 1G */
static uint64_t parse_size(const char *str)
{
    char *end;
    uint64_t n = strtoull(str, &end, 0);

    switch (*end)
    {
    case 'k':
    case 'K':
        n <<= 10;
        end++;
        break;
    case 'm':
    case 'M':
        n <<= 20;
        end++;
        break;
    case 'g':
    case 'G':
        n <<= 30;
        end++;
        break;
    }
    if (end == str || *end != '\0' || n == 0 || n > RAM_SIZE_MAX || (n & 3))
    {
        fprintf(stderr, "%s: invalid ram size '%s' (1..4G, multiple of 4, optional K/M/G suffix)\n", PROGRAM_NAME, str);
        exit(EXIT_FAILURE);
    }
    return n;
}

static uint32_t bus_read(cpu_t *cpu, uint32_t addr, uint32_t size)
{
    uint32_t v = 0, i;
    if (addr < ram_size && size <= ram_size - addr)
    {
        for (i = 0; i < size; i++)
            v |= (uint32_t)ram[addr + i] << (8 * i);
        return v;
    }
    fprintf(stderr, "[bus] read fault at 0x%08X\n", addr);
    cpu->halted = 1;
    return 0;
}

static void bus_write(cpu_t *cpu, uint32_t addr, uint32_t value, uint32_t size)
{
    uint32_t i;

    if (addr == 0xFFF0 && size == 1)
    { // uart tx, byte access only (non ISA)
        putc((int)(value & 0xFF), stdout);
        fflush(stdout);
        return;
    }

    if (addr < ram_size && size <= ram_size - addr)
    {
        for (i = 0; i < size; i++)
            ram[addr + i] = (uint8_t)(value >> (8 * i));
        return;
    }
    fprintf(stderr, "[bus] write fault at 0x%08X\n", addr);
    cpu->halted = 1;
}

static void cpu_trap(cpu_t *cpu, uint32_t reason, uint32_t word)
{
    fprintf(stderr, "[trap] reason=%u word=0x%08X pc=0x%08X\n",
            reason, word, cpu->pc - 4);
    cpu->halted = 1;
}

static const char *reg_names[REG_COUNT] = {
    "x0", "rp", "sp", "fp",
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
    "r24", "r25", "r26", "r27"};

static void cpu_dump(const cpu_t *cpu)
{
    int i;
    printf("pc     = 0x%08X\n", cpu->pc);
    printf("flags  = 0x%08X [%c%c%c%c%c]\n",
           cpu->flags,
           (cpu->flags & FLAG_Z) ? 'Z' : '-',
           (cpu->flags & FLAG_N) ? 'N' : '-',
           (cpu->flags & FLAG_C) ? 'C' : '-',
           (cpu->flags & FLAG_O) ? 'O' : '-',
           (cpu->flags & FLAG_IE) ? 'I' : '-');
    printf("halted = %d\n", cpu->halted);
    for (i = 0; i < REG_COUNT; i++)
    {
        printf("%-3s = 0x%08X", reg_names[i], cpu->regs[i]);
        printf((i % 4 == 3) ? "\n" : "  ");
    }
}

/* runtime disassembler */
static const char *alu_names[] = {
    "add", "sub", "mul", "rem", "sdiv", "udiv",
    "and", "or", "xor", "shl", "shr"};
static const char *mem_names[] = {"b", "h", "w"};
static const char *br_names[] = {"", ".eq", ".ne", ".lt", ".gt", ".le", ".ge"};

#define ALU_NAME(f) ((f) <= ALU_SHR ? alu_names[f] : "???")
#define MEM_NAME(f) ((f) <= MEM_WORD ? mem_names[f] : "?")
#define BR_NAME(f) ((f) <= BR_GE ? br_names[f] : ".???")

static void disasm(cpu_t *cpu, uint32_t word, char *buf, size_t len)
{
    sra32_instr_t in;
    const char *rd, *rt, *rs;

    sra32_decode(cpu, word, &in);
    rd = reg_names[in.rd & 0x1F];
    rt = reg_names[in.rt & 0x1F];
    rs = reg_names[in.rs & 0x1F];

    switch (in.op)
    {
    case 0x00:
        snprintf(buf, len, "nop");
        break;
    case 0x01:
        snprintf(buf, len, "alu.%s %s, %s, %s", ALU_NAME(in.funct), rd, rt, rs);
        break;
    case 0x02:
        snprintf(buf, len, "mov %s, %s", rd, rs);
        break;
    case 0x03:
        snprintf(buf, len, "cmp %s, %s", rd, rs);
        break;
    case 0x04:
        snprintf(buf, len, "push %s", rd);
        break;
    case 0x05:
        snprintf(buf, len, "pop %s", rd);
        break;
    case 0x06:
        snprintf(buf, len, "rfl %s", rd);
        break;
    case 0x07:
        snprintf(buf, len, "wfl %s", rd);
        break;
    case 0x08:
        snprintf(buf, len, "spc %s", rd);
        break;
    case 0x09:
        snprintf(buf, len, "calr %s", rd);
        break;
    case 0x0A:
        snprintf(buf, len, "alui.%s %s, %s, %d", ALU_NAME(in.funct), rd, rt, in.simm);
        break;
    case 0x0B:
        snprintf(buf, len, "l%s %s, %d[%s]", MEM_NAME(in.funct), rd, in.simm, rt);
        break;
    case 0x0C:
        snprintf(buf, len, "s%s %s, %d[%s]", MEM_NAME(in.funct), rd, in.simm, rt);
        break;
    case 0x0F:
        snprintf(buf, len, "sui %s, 0x%X", rd, in.imm);
        break;
    case 0x10:
        snprintf(buf, len, "br%s 0x%08X (%+d)", BR_NAME(in.funct), in.pc + (uint32_t)in.simm, in.simm);
        break;
    case 0x11:
        snprintf(buf, len, "call 0x%08X (%+d)", in.pc + (uint32_t)in.simm, in.simm);
        break;
    default:
        if (sra32_ops[in.op].handler)
            snprintf(buf, len, "%s (custom)", sra32_ops[in.op].mnemonic);
        else if (sra32_ops[in.op].mnemonic[0])
            snprintf(buf, len, "%s (unimplemented)", sra32_ops[in.op].mnemonic);
        else
            snprintf(buf, len, "???");
        break;
    }
}

static void print_instr(cpu_t *cpu)
{
    uint32_t word = cpu->read(cpu, cpu->pc, 4);
    char buf[64];
    if (cpu->halted)
        return;
    disasm(cpu, word, buf, sizeof(buf));
    printf("0x%08X: %08X  %s\n", cpu->pc, word, buf);
}

static uint64_t run(cpu_t *cpu)
{
    uint64_t steps = 0;
    char line[16];

    if (!opts.step && !opts.dump_step && !opts.disasm)
        return sra32_run(cpu, opts.max);

    while (!cpu->halted && (opts.max == 0 || steps < opts.max))
    {
        if (opts.step || opts.disasm)
            print_instr(cpu);

        if (opts.step)
        {
            printf("(step) ");
            fflush(stdout);
            if (!fgets(line, sizeof(line), stdin) || line[0] == 'q')
                break;
        }

        sra32_step(cpu);
        steps++;
        if (opts.dump_step)
        {
            printf("--- step %llu ---\n", (unsigned long long)steps);
            cpu_dump(cpu);
        }
    }
    return steps;
}

/* experimental elf32 loader, just enough to run static executables
 * from the sra32 binutils/gcc port: parse the program headers, copy
 * every PT_LOAD segment into ram and return the entry point */
#define ELF_PT_LOAD 1
#define ELF_EHDR_SIZE 52
#define ELF_PHDR_SIZE 32

/* explicit little-endian readers so the loader is correct no matter
 * what endianness the host has (sra32 is always lil endian) */
static uint16_t rd16(const uint8_t *p)
{
    return (uint16_t)(p[0] | (p[1] << 8));
}
static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void elf_die(const char *path, const char *why)
{
    fprintf(stderr, "%s: '%s': %s\n", PROGRAM_NAME, path, why);
    exit(EXIT_FAILURE);
}

static uint32_t load_elf(const char *path)
{
    FILE *f = fopen(path, "rb");
    uint8_t *file;
    long file_size;
    uint32_t e_entry, e_phoff;
    uint16_t e_phentsize, e_phnum;
    int i, loaded = 0;
    fprintf(stderr, "[elf] WARNING: the ELF loader is experimental\n");
    if (!f)
    {
        fprintf(stderr, "%s: cannot open '%s'\n", PROGRAM_NAME, path);
        exit(EXIT_FAILURE);
    }

    fseek(f, 0, SEEK_END);
    file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (file_size < ELF_EHDR_SIZE)
        elf_die(path, "file too small to be an ELF");

    file = malloc((size_t)file_size);
    if (!file || fread(file, 1, (size_t)file_size, f) != (size_t)file_size)
        elf_die(path, "read failed");
    fclose(f);

    if (file[0] != 0x7F || file[1] != 'E' || file[2] != 'L' || file[3] != 'F')
        elf_die(path, "not an ELF file (bad magic)");
    if (file[4] != 1) // EI_CLASS = ELFCLASS32
        elf_die(path, "not a 32-bit ELF");
    if (file[5] != 1) // EI_DATA = ELFDATA2LSB, sra32 is little endian
        elf_die(path, "not little-endian");

    e_entry = rd32(file + 24);
    e_phoff = rd32(file + 28);
    e_phentsize = rd16(file + 42);
    e_phnum = rd16(file + 44);

    if (e_phnum == 0 || e_phentsize != ELF_PHDR_SIZE)
        elf_die(path, "no usable program headers");
    if ((uint64_t)e_phoff + (uint64_t)e_phnum * ELF_PHDR_SIZE > (uint64_t)file_size)
        elf_die(path, "program headers out of bounds");

    for (i = 0; i < e_phnum; i++)
    {
        const uint8_t *p = file + e_phoff + (size_t)i * ELF_PHDR_SIZE;
        uint32_t p_type = rd32(p + 0);
        uint32_t p_offset = rd32(p + 4);
        uint32_t p_vaddr = rd32(p + 8);
        uint32_t p_filesz = rd32(p + 16);
        uint32_t p_memsz = rd32(p + 20);

        if (p_type != ELF_PT_LOAD || p_memsz == 0)
            continue;

        if ((uint64_t)p_vaddr + p_memsz > ram_size)
            elf_die(path, "PT_LOAD segment does not fit in ram (try --ram-size)");
        if ((uint64_t)p_offset + p_filesz > (uint64_t)file_size)
            elf_die(path, "PT_LOAD segment out of file bounds");
        if (p_filesz > p_memsz)
            elf_die(path, "PT_LOAD filesz > memsz");

        memcpy(ram + p_vaddr, file + p_offset, p_filesz);
        memset(ram + p_vaddr + p_filesz, 0, p_memsz - p_filesz); // .bss
        fprintf(stderr, "[elf] load 0x%08X..0x%08X (%u bytes, %u from file)\n", p_vaddr, p_vaddr + p_memsz, p_memsz, p_filesz);
        loaded++;
    }
    if (!loaded)
        elf_die(path, "no PT_LOAD segments");

    fprintf(stderr, "[elf] entry = 0x%08X\n", e_entry);
    free(file);
    return e_entry;
}

static size_t load_rom(const char *path)
{
    FILE *f = fopen(path, "rb");
    size_t n;

    if (!f)
    {
        fprintf(stderr, "%s: cannot open '%s'\n", PROGRAM_NAME, path);
        exit(EXIT_FAILURE);
    }

    n = fread(ram, 1, ram_size, f);
    if (fgetc(f) != EOF)
    {
        fprintf(stderr, "%s: rom '%s' is larger than RAM (%llu bytes)\n", PROGRAM_NAME, path, (unsigned long long)ram_size);
        fclose(f);
        exit(EXIT_FAILURE);
    }
    fclose(f);

    if (n == 0)
    {
        fprintf(stderr, "%s: rom '%s' is empty\n", PROGRAM_NAME, path);
        exit(EXIT_FAILURE);
    }

    if (n >= 4 && ram[0] == 0x7F && ram[1] == 'E' && ram[2] == 'L' && ram[3] == 'F')
        fprintf(stderr, "%s: warning: '%s' looks like an ELF file, did you mean --elf?\n", PROGRAM_NAME, path);
    return n;
}

static void usage(int status)
{
    FILE *out = status == EXIT_SUCCESS ? stdout : stderr;
    fprintf(out, "Usage: %s [OPTION]... <rom>\n", PROGRAM_NAME);
    fprintf(out, "Run an SRA32 rom image.\n\n");
    fprintf(out, "  -s, --step        interactive step mode (enter = step, q = quit)\n");
    fprintf(out, "  -x, --disasm      disassemble every instruction during runtime\n");
    fprintf(out, "  -e, --elf         load the rom as an ELF executable (EXPERIMENTAL)\n");
    fprintf(out, "  -d, --dump-step   dump cpu state after every step\n");
    fprintf(out, "  -D, --dump-end    dump cpu state when execution finishes\n");
    fprintf(out, "  -m, --max=N       stop after N steps (default: unlimited)\n");
    fprintf(out, "  -r, --ram-size=N  set ram size, K/M/G suffixes allowed (default: 64K)\n");
    fprintf(out, "  -h, --help        display this help and exit\n");
    fprintf(out, "  -V, --version     output version information and exit\n");
    exit(status);
}

static void version(void)
{
    printf("%s %s\n", PROGRAM_NAME, PROGRAM_VERSION);
    printf("Written by %s.\n", PROGRAM_AUTHOR);
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
    cpu_t cpu;
    uint64_t steps;
    uint32_t entry;
    size_t rom_size;
    int c;

    static const struct option long_opts[] = {
        {"step", no_argument, NULL, 's'},
        {"disasm", no_argument, NULL, 'x'},
        {"elf", no_argument, NULL, 'e'},
        {"dump-step", no_argument, NULL, 'd'},
        {"dump-end", no_argument, NULL, 'D'},
        {"max", required_argument, NULL, 'm'},
        {"ram-size", required_argument, NULL, 'r'},
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'V'},
        {NULL, 0, NULL, 0}};

    while ((c = getopt_long(argc, argv, "sxedDm:r:hV", long_opts, NULL)) != -1)
    {
        switch (c)
        {
        case 's':
            opts.step = 1;
            break;
        case 'x':
            opts.disasm = 1;
            break;
        case 'e':
            opts.elf = 1;
            break;
        case 'd':
            opts.dump_step = 1;
            break;
        case 'D':
            opts.dump_end = 1;
            break;
        case 'm':
            opts.max = strtoull(optarg, NULL, 0);
            break;
        case 'r':
            ram_size = parse_size(optarg);
            break;
        case 'h':
            usage(EXIT_SUCCESS);
            break;
        case 'V':
            version();
            break;
        default:
            usage(EXIT_FAILURE);
        }
    }

    if (optind != argc - 1)
        usage(EXIT_FAILURE);

    ram = calloc(1, ram_size);
    atexit(cleanup);
    if (!ram)
    {
        fprintf(stderr, "%s: failed to allocate %llu bytes of ram\n",
                PROGRAM_NAME, (unsigned long long)ram_size);
        return EXIT_FAILURE;
    }

    entry = 0;
    rom_size = 0;
    if (opts.elf)
        entry = load_elf(argv[optind]);
    else
        rom_size = load_rom(argv[optind]);

    sra32_init(&cpu, bus_read, bus_write, cpu_trap, NULL);
    sra32_reset(&cpu, entry);

    steps = run(&cpu);

    if (opts.elf)
        printf("halted after %llu steps\n", (unsigned long long)steps);
    else
        printf("halted after %llu steps (rom: %zu bytes)\n",
               (unsigned long long)steps, rom_size);
    if (opts.dump_end)
        cpu_dump(&cpu);

    return EXIT_SUCCESS;
}