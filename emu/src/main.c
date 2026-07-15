#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sra32/bus.h>
#include <sra32/cpu.h>
#include <sra32/isa.h>
#include <util/bit.h>
#include <util/log.h>

#define MEM_SIZE (64 * 1024)
#define LOAD_BASE 0x00000000u

typedef struct options
{
    const char *image;
    bool disasm_only;
    bool trace;
    bool dump_regs;
    uint64_t max_steps;
} options_t;

static void print_usage(const char *argv0)
{
    fprintf(stderr,
            "usage: %s [options] <image>\n"
            "\n"
            "arguments:\n"
            "  image              ROM image to load at 0x%08x\n"
            "\n"
            "options:\n"
            "  -d, --disasm       disassemble the ROM instead of running it\n"
            "  -t, --trace        log every executed instruction\n"
            "  -r, --regs         dump all registers after the run\n"
            "  -m, --max-steps N  stop after N instructions (0 = unlimited)\n"
            "  -h, --help         show this help\n",
            argv0, LOAD_BASE);
}

static bool parse_args(int argc, char **argv, options_t *opts)
{
    for (int i = 1; i < argc; i++)
    {
        const char *arg = argv[i];
        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0)
        {
            print_usage(argv[0]);
            return false;
        }
        else if (strcmp(arg, "-d") == 0 || strcmp(arg, "--disasm") == 0)
        {
            opts->disasm_only = true;
        }
        else if (strcmp(arg, "-t") == 0 || strcmp(arg, "--trace") == 0)
        {
            opts->trace = true;
        }
        else if (strcmp(arg, "-r") == 0 || strcmp(arg, "--regs") == 0)
        {
            opts->dump_regs = true;
        }
        else if (strcmp(arg, "-m") == 0 || strcmp(arg, "--max-steps") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "%s requires a value\n", arg);
                print_usage(argv[0]);
                return false;
            }

            char *end = NULL;
            opts->max_steps = strtoull(argv[++i], &end, 0);
            if (!end || *end != '\0')
            {
                fprintf(stderr, "invalid step count: %s\n", argv[i]);
                return false;
            }
        }
        else if (arg[0] != '-' && arg[0] != '\0' && !opts->image)
        {
            opts->image = arg;
        }
        else
        {
            fprintf(stderr, "unknown option: %s\n", arg);
            print_usage(argv[0]);
            return false;
        }
    }
    return true;
}

static uint8_t *load_rom(const options_t *opts, size_t *out_size)
{
    FILE *file = fopen(opts->image, "rb");
    if (!file)
    {
        log_error("failed to open ROM image: %s", opts->image);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    const long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size <= 0)
    {
        log_error("ROM image is empty: %s", opts->image);
        fclose(file);
        return NULL;
    }
    if ((size_t)size > MEM_SIZE - LOAD_BASE)
    {
        log_error("ROM image does not fit in memory: %s", opts->image);
        fclose(file);
        return NULL;
    }
    if (size % 4 != 0)
        log_warn("ROM image size is not a multiple of 4 bytes");

    uint8_t *rom = malloc((size_t)size);
    if (!rom || fread(rom, 1, (size_t)size, file) != (size_t)size)
    {
        log_error("failed to read ROM image: %s", opts->image);
        free(rom);
        fclose(file);
        return NULL;
    }

    fclose(file);
    *out_size = (size_t)size;
    return rom;
}

static void disassemble(const isa_t *isa, const uint8_t *rom, size_t size)
{
    for (size_t i = 0; i + 4 <= size; i += 4)
    {
        const uint32_t word = LE32_LOAD(&rom[i]);
        const uint32_t pc = LOAD_BASE + (uint32_t)i;
        const decoded_t in = instr_decode(pc, word);

        char buf[128];
        isa_disasm(isa, &in, buf, sizeof buf);
        printf("%08x:  %08x  %s\n", pc, word, buf);
    }
}

int main(int argc, char **argv)
{
    options_t opts = {0};
    if (!parse_args(argc, argv, &opts))
        return 1;

    if (!opts.image)
    {
        fprintf(stderr, "no ROM image given\n");
        print_usage(argv[0]);
        return 1;
    }

    if (opts.trace)
        log_set_level(LOG_TRACE);

    isa_t isa;
    if (!isa_init(&isa))
        return 1;

    size_t rom_size = 0;
    uint8_t *rom = load_rom(&opts, &rom_size);
    if (!rom)
        return 1;

    if (opts.disasm_only)
    {
        disassemble(&isa, rom, rom_size);
        free(rom);
        return 0;
    }

    ram_t ram;
    if (!ram_init(&ram, MEM_SIZE))
    {
        free(rom);
        return 1;
    }
    ram_load(&ram, rom, rom_size, LOAD_BASE);
    free(rom);

    cpu_t cpu;
    cpu_init(&cpu, &ram.bus, &isa);
    cpu_reset(&cpu, LOAD_BASE);
    cpu_set_reg(&cpu, REG_SP, MEM_SIZE);

    const uint64_t retired = cpu_run(&cpu, opts.max_steps);
    if (!cpu.halted)
        log_info("max steps reached");
    log_info("retired %llu instruction(s), pc=0x%08x", (unsigned long long)retired, cpu.pc);

    if (opts.dump_regs)
        cpu_dump(&cpu, stdout);

    ram_destroy(&ram);
    return 0;
}
