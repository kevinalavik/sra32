#include <sra32ka/device.h>
#include <sra32ka/asm.h>

#define SERIAL_DATA 0x00
#define SERIAL_STATUS 0x04
#define SERIAL_STATUS_TX_READY (1u << 1)

static sra32ka_device_t serial0;
static int have_serial0;

static void serial_putc(uint32_t c)
{
    if (!have_serial0)
        return;

    while (!(sra32ka_device_read32(&serial0, SERIAL_STATUS) & SERIAL_STATUS_TX_READY))
        ;

    sra32ka_device_write32(&serial0, SERIAL_DATA, c);
}

static void serial_puts(const char *s)
{
    while (*s)
    {
        if (*s == '\n')
            serial_putc('\r');
        serial_putc(*s);
        s++;
    }
}

static void print_device_name(uint32_t index)
{
    uint32_t i;

    for (i = 0; i < SRA32KA_DEVICE_NAME_MAX; i++)
    {
        uint32_t c = sra32ka_entry_name_byte(index, i);
        if (c == 0)
            break;
        serial_putc(c);
    }
}

static void print_device_tree(uint32_t index, uint32_t depth)
{
    uint32_t count = sra32ka_device_count();
    uint32_t i;

    for (i = 0; i < depth; i++)
        serial_puts("  ");
    if (depth != 0)
        serial_puts("|-");
    print_device_name(index);
    serial_puts("\n");

    for (i = 0; i < count; i++)
    {
        sra32ka_device_t child;
        child.index = i;
        if (sra32ka_device_parent(&child) == index)
            print_device_tree(i, depth + 1);
    }
}

void boot()
{
    have_serial0 = (sra32ka_device_open("sra32ka-host.serial.serial0", &serial0) == 0);
    serial_puts("================================================\n");
    serial_puts("    SRA32 (sra32ka-v1.0) Firmware Rev. 1\n");
    serial_puts("            Written by Kevin Alavik\n");
    serial_puts("\nDevice tree:\n");
    serial_puts("--------\n");
    print_device_tree(0, 0);
    serial_puts("================================================\n");
    serial_puts("warning: No bootable devices yet :(\n");
}
